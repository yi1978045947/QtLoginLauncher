#include "cef_web_bridge.h"

namespace qtlogin::sdologin {

std::string cefBridgeObjectName()
{
    return "qtlogin";
}

std::string cefBridgePostMessageFunctionName()
{
    return "postMessage";
}

std::string cefBridgeOnMessageFunctionName()
{
    return "onMessage";
}

std::string cefBridgeWebToNativeMessageName()
{
    return "qtlogin.webToNative";
}

std::string cefBridgeNativeToWebMessageName()
{
    return "qtlogin.nativeToWeb";
}

}
