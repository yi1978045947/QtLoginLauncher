#include <cassert>
#include <iostream>

#include "json_payload.h"
#include "protocol_frame.h"

using qtlogin::protocol::Message;
using qtlogin::protocol::MessageType;

static void assertRoundTrip(MessageType type)
{
    Message input;
    input.type = type;
    input.requestId = 42;
    input.fields["sessionId"] = "mock-session";
    input.fields["sndaid"] = "mock-user";
    input.fields["text"] = "quote\"slash\\line\n";

    const auto bytes = serializeMessage(input);
    Message output;
    assert(parseMessage(bytes, &output));
    assert(output.type == input.type);
    assert(output.requestId == input.requestId);
    assert(output.fields == input.fields);
}

int main()
{
    assertRoundTrip(MessageType::Hello);
    assertRoundTrip(MessageType::HelloAck);
    assertRoundTrip(MessageType::ShowLogin);
    assertRoundTrip(MessageType::LoginSuccess);
    assertRoundTrip(MessageType::Shutdown);

    std::cout << "protocol_tests passed\n";
    return 0;
}
