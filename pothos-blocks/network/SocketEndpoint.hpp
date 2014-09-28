//
// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0
//

#pragma once
#include <Pothos/Config.hpp>
#include <Pothos/Framework/BufferChunk.hpp>
#include <Poco/Timespan.h>
#include <Poco/Types.h>

static const Poco::UInt16 PothosPacketTypeMessage = Poco::UInt16('M');
static const Poco::UInt16 PothosPacketTypeLabel = Poco::UInt16('L');
static const Poco::UInt16 PothosPacketTypeBuffer = Poco::UInt16('B');
static const Poco::UInt16 PothosPacketTypeDType = Poco::UInt16('D');

class PothosPacketSocketEndpoint
{
public:

    /*!
     * Create a new socket endpoint.
     * For the URI scheme, the protocol can be udp or tcp.
     * Do not specify the port for automatic port selection on BIND.
     * \param uri the socket parameters proto://host:port
     * \param opt the socket mode BIND or CONNECT
     */
    PothosPacketSocketEndpoint(const std::string &uri, const std::string &opt);

    /*!
     * Destruct a socket endpoint.
     */
    ~PothosPacketSocketEndpoint(void);

    /*!
     * Get the actual port of the socket.
     * Use this to discover the port of an endpoint,
     * with an unspecified port number in the bind URL.
     */
    std::string getActualPort(void) const;

    /*!
     * Perform the communication initialization handshake.
     */
    void openComms(void);

    /*!
     * Perform the communication shutdown handshake.
     */
    void closeComms(void);

    /*!
     * Is the endpoint ready for communication?
     */
    bool isReady(void);

    /*!
     * Receive data from the remote endpoint.
     */
    void recv(Poco::UInt16 &type, Poco::UInt64 &index, Pothos::BufferChunk &buffer, const Poco::Timespan &timeout = Poco::Timespan(Poco::Timespan::TimeDiff(1e6*0.05)));

    /*!
     * Send data to the remote endpoint.
     */
    void send(const Poco::UInt16 type, const Poco::UInt64 &index, const void *buff, const size_t numBytes);

private:
    struct Impl; Impl *_impl;
};
