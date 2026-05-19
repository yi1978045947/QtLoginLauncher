#pragma once

#include <cstdint>
#include <vector>

#include "protocol_types.h"

namespace qtlogin::protocol {

#pragma pack(push, 1)
struct FrameHeader {
    uint32_t magic = kFrameMagic;
    uint16_t version = kProtocolVersion;
    uint16_t type = 0;
    uint32_t flags = 0;
    uint64_t requestId = 0;
    uint32_t payloadSize = 0;
};
#pragma pack(pop)

static_assert(sizeof(FrameHeader) == 24, "FrameHeader must remain stable");

std::vector<uint8_t> serializeMessage(const Message& message);
bool parseMessage(const std::vector<uint8_t>& bytes, Message* message);

}
