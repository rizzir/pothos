########################################################################
# Packet blocks module
########################################################################
POTHOS_MODULE_UTIL(
    TARGET PacketBlocks
    SOURCES
        SimpleMac.cpp
        PacketToStream.cpp
        StreamToPacket.cpp
        TestPacketBlocks.cpp
        LabelDeframer.cpp
    DESTINATION blocks
    ENABLE_DOCS
)
