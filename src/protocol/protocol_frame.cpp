#include "protocol_frame.h"

#include <cstring>
#include <limits>
#include <utility>

#include "json_payload.h"

namespace qtlogin::protocol {

std::vector<uint8_t> serializeMessage(const Message& message)
{
    const std::string payload = encodeJsonPayload(message.fields);

    FrameHeader header;
    header.type = static_cast<uint16_t>(message.type);
    header.requestId = message.requestId;
    header.payloadSize = static_cast<uint32_t>(payload.size());

    std::vector<uint8_t> bytes(sizeof(FrameHeader) + payload.size());
    std::memcpy(bytes.data(), &header, sizeof(header));
    if (!payload.empty()) {
        std::memcpy(bytes.data() + sizeof(FrameHeader), payload.data(), payload.size());
    }
    return bytes;
}

bool parseMessage(const std::vector<uint8_t>& bytes, Message* message)
{
    if (!message || bytes.size() < sizeof(FrameHeader)) {
        return false;
    }

    FrameHeader header;
    std::memcpy(&header, bytes.data(), sizeof(header));
    if (header.magic != kFrameMagic || header.version != kProtocolVersion) {
        return false;
    }
    if (header.payloadSize > bytes.size() - sizeof(FrameHeader)) {
        return false;
    }
    if (sizeof(FrameHeader) + header.payloadSize != bytes.size()) {
        return false;
    }

    std::string payload;
    if (header.payloadSize > 0) {
        payload.assign(reinterpret_cast<const char*>(bytes.data() + sizeof(FrameHeader)), header.payloadSize);
    }

    Message parsed;
    parsed.type = static_cast<MessageType>(header.type);
    parsed.requestId = header.requestId;
    if (!decodeJsonPayload(payload.empty() ? "{}" : payload, &parsed.fields)) {
        return false;
    }

    *message = std::move(parsed);
    return true;
}

}
