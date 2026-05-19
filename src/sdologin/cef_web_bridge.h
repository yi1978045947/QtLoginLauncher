#pragma once

#include <string>

namespace qtlogin::sdologin {

std::string cefBridgeObjectName();
std::string cefBridgePostMessageFunctionName();
std::string cefBridgeOnMessageFunctionName();
std::string cefBridgeWebToNativeMessageName();
std::string cefBridgeNativeToWebMessageName();

}
