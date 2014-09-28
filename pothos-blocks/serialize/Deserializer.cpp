// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "SerializeCommon.hpp"
#include <Pothos/Framework.hpp>
#include <Poco/ByteOrder.h>
#include <Poco/Format.h>
#include <sstream>
#include <cstring>
#include <cassert>

/***********************************************************************
 * |PothosDoc Deserializer
 *
 * Deserialize the contents of a mVRL stream into 1 or more output ports.
 * The mVRL stream encapsulates input streams, labels, and messages.
 * The streaming data is restored in its original binary format.
 * The contents of the labels and messages are deserialized.
 * The deserializer inputs a byte stream containing mVRL on port 0.
 * The output ports are indexed starting at 0 and incrementing.
 *
 * The modified VITA radio link layer (mVRL) protocol encapsulates packets,
 * preserving both the packet order and the packet boundaries.
 * A VRL stream can be stored to file, or sent across a network.
 * In the event of loss, the bounds of the stream can be recovered.
 *
 * |category /Serialize
 * |keywords deserialize serialize VRL
 *
 * |factory /blocks/deserializer()
 **********************************************************************/
class Deserializer : public Pothos::Block
{
public:
    Deserializer(void):
        _nextExpectedIndex(0)
    {
        this->setupInput(0);
        this->setupOutput(0);
    }

    static Block *make(void)
    {
        return new Deserializer();
    }

    void work(void);
    void handlePacket(const Pothos::BufferChunk &);

private:
    Pothos::BufferChunk _accumulator;
    unsigned long long _nextExpectedIndex;
};

static Pothos::BlockRegistry registerDeserializer(
    "/blocks/deserializer", &Deserializer::make);

/*!
 * Inspect an input buffer for an entire valid packet.
 */
static bool inspectPacket(const Pothos::BufferChunk &packet, bool &isFragment, size_t &pkt_bytes)
{
    auto vrlp_pkt = packet.as<const Poco::UInt32 *>();
    auto p = packet.as<const char *>();
    if ((p[0] == 'm') and (p[1] == 'V') and (p[2] == 'R') and (p[3] == 'L'))
    {
        assert(Poco::ByteOrder::fromNetwork(vrlp_pkt[0]) == mVRL);
        pkt_bytes = Poco::ByteOrder::fromNetwork(vrlp_pkt[1]) & 0xfffff;
        const size_t pkt_words32 = padUp32(pkt_bytes)/4;
        isFragment = pkt_bytes > packet.length;
        if (pkt_bytes > MAX_PKT_BYTES) return false; //call this BS
        return isFragment or Poco::ByteOrder::fromNetwork(vrlp_pkt[pkt_words32-1]) == VEND;
    }
    return false;
}

/*!
 * Unpack a buffer containing a packet into the header and payload contents.
 */
static void unpackBuffer(const Pothos::BufferChunk &packet, size_t &seq, size_t &sid, bool &has_tsf, unsigned long long &tsf, bool &is_ext, Pothos::BufferChunk &payloadBuff)
{
    #define unpackCheck(cond) if (not (cond)) throw Pothos::AssertionViolationException("Deserializer::unpackBuffer()", "failed assertion: " #cond)
    auto p = packet.as<const Poco::UInt32 *>();

    //validate vrlp
    assert(Poco::ByteOrder::fromNetwork(p[0]) == mVRL);
    const size_t pkt_bytes = Poco::ByteOrder::fromNetwork(p[1]) & 0xfffff;
    const size_t pkt_words32 = padUp32(pkt_bytes)/4;
    assert(Poco::ByteOrder::fromNetwork(p[pkt_words32-1]) == VEND);
    unpackCheck(pkt_words32*4 <= packet.length);
    const size_t seq12 = Poco::ByteOrder::fromNetwork(p[1]) >> 20;

    //validate vita
    const auto vita_hdr = Poco::ByteOrder::fromNetwork(p[2]);
    const size_t vita_words32 = vita_hdr & 0xffff;
    unpackCheck(vita_words32 == pkt_words32 - 3);

    //validate seq
    const size_t seq4 = (vita_hdr >> 16) & 0xf;
    unpackCheck((seq12 & 0x4) == seq4);

    has_tsf = bool(vita_hdr & VITA_TSF);
    unpackCheck(bool(vita_hdr & VITA_SID));
    is_ext = bool(vita_hdr & VITA_EXT);

    //assert other fields are blank - expected
    unpackCheck((vita_hdr & (1 << 30)) == 0);
    unpackCheck((vita_hdr & (1 << 27)) == 0);
    unpackCheck((vita_hdr & (1 << 26)) == 0);
    unpackCheck((vita_hdr & (1 << 23)) == 0);
    unpackCheck((vita_hdr & (1 << 22)) == 0);

    //extract seq and sid
    seq = seq12;
    sid = Poco::ByteOrder::fromNetwork(p[3]);

    //only valid when has_tsf
    tsf = (Poco::UInt64(Poco::ByteOrder::fromNetwork(p[4])) << 32) | Poco::ByteOrder::fromNetwork(p[5]);

    //set out buff
    const size_t hdr_words32 = has_tsf? 6 : 4;
    payloadBuff = packet;
    payloadBuff.address += hdr_words32*4;
    payloadBuff.length = pkt_bytes - hdr_words32*4 - 4;
}

void Deserializer::work(void)
{
    auto inputPort = this->input(0);
    auto buff = inputPort->buffer();
    inputPort->consume(buff.length);

    //TODO use an iovec approach to avoid accumulating

    _accumulator.append(buff);

    //character by character recovery search for packet header
    while (_accumulator.length >= MIN_PKT_BYTES)
    {
        bool isFragment = true; size_t pkt_bytes = 0;
        if (inspectPacket(_accumulator, isFragment, pkt_bytes))
        {
            if (isFragment) return; //wait for more incoming buffers to accumulate
            this->handlePacket(_accumulator); //handle the packet, its good probably

            //increment for the next iteration
            assert(pkt_bytes <= _accumulator.length);
            _accumulator.address += pkt_bytes;
            _accumulator.length -= pkt_bytes;
            assert(_accumulator.length <= _accumulator.getBuffer().getLength());
        }
        else
        {
            //the search continues
            _accumulator.address++;
            _accumulator.length--;
        }
    }

    //dont keep a reference if the buffer is empty
    if (_accumulator.length == 0) _accumulator = Pothos::BufferChunk();
}

/*!
 * Top level handler logic for a buffer containing a mVRL frame.
 */
void Deserializer::handlePacket(const Pothos::BufferChunk &packetBuff)
{
    //extract info
    size_t seq = 0;
    size_t sid = 0;
    bool has_tsf = false;
    unsigned long long tsf = 0;
    bool is_ext = false;
    Pothos::BufferChunk payloadBuff;
    unpackBuffer(packetBuff, seq, sid, has_tsf, tsf, is_ext, payloadBuff);

    if (sid >= this->outputs().size()) throw Pothos::RangeException("Deserializer::handlePacket()",
        Poco::format("packet has SID %d, but block has %d outputs", int(sid), int(this->outputs().size())));
    auto outputPort = this->output(sid);

    //handle buffs
    if (not is_ext)
    {
        assert(has_tsf);
        _nextExpectedIndex = tsf + payloadBuff.length;
        outputPort->postBuffer(payloadBuff);
    }

    else
    {
        std::stringstream ss(std::string(payloadBuff.as<const char *>(), payloadBuff.length));
        Pothos::Object obj;
        obj.deserialize(ss);

        //handle labels
        if (has_tsf)
        {
            auto lbl = obj.extract<Pothos::Label>();
            lbl.index = tsf - _nextExpectedIndex;
            outputPort->postLabel(lbl);
        }

        //handle msgs
        else outputPort->postMessage(obj);
    }
}
