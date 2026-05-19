#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace qtlogin::protocol {

constexpr uint16_t kProtocolVersion = 1;
constexpr uint32_t kFrameMagic = 0x31504C51; // QLP1 in little endian.

enum class MessageType : uint16_t {
    Hello = 1,
    HelloAck = 2,
    SetOwnerWindow = 3,
    ShowLogin = 4,
    CloseLogin = 5,
    LoginSuccess = 6,
    LoginCancel = 7,
    LoginError = 8,
    GetTicket = 9,
    GetTicketResult = 10,
    ChallengeRequired = 11,
    ChallengeResult = 12,
    OpenWebDialog = 13,
    WebDialogResult = 14,
    Logout = 15,
    Heartbeat = 16,
    Shutdown = 17
};

struct Message {
    MessageType type = MessageType::Hello;
    uint64_t requestId = 0;
    std::map<std::string, std::string> fields;
};

}
