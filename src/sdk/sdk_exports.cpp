#include <windows.h>

#include "legacy/interface.h"
#include "SDOA4Client.h"
#include "directx_compat_adapter.h"
#include "legacy_login_adapter.h"
#include "sdoa_app_adapter.h"
#include "sdk_runtime.h"

extern "C" int WINAPI SDOLInitialize(const SDOLAppInfo* appInfo)
{
    return qtlogin::sdk::SdkRuntime::instance().initialize(appInfo);
}

extern "C" int WINAPI SDOLGetModule(REFIID riid, void** intf)
{
    if (!intf) {
        return SDOL_ERRORCODE_INVALIDPARAM;
    }
    *intf = nullptr;
    if (SUCCEEDED(qtlogin::sdk::LegacyLoginAdapter::instance().QueryInterface(riid, intf))) {
        return SDOL_ERRORCODE_OK;
    }
    if (qtlogin::sdk::queryDirectXCompatModule(riid, intf)) {
        return SDOL_ERRORCODE_OK;
    }
    return SDOL_ERRORCODE_NOT_SUPPORT;
}

extern "C" int WINAPI SDOLTerminal()
{
    return qtlogin::sdk::SdkRuntime::instance().terminal();
}

extern "C" int WINAPI igwInitialize(DWORD sdkVersion, const AppInfo* appInfo)
{
    return qtlogin::sdk::initializeSdoaRuntime(sdkVersion, appInfo);
}

extern "C" bool WINAPI igwGetModule(REFIID riid, void** intf)
{
    if (!intf) {
        return false;
    }
    *intf = nullptr;
    if (qtlogin::sdk::querySdoaModule(riid, intf)) {
        return true;
    }
    return qtlogin::sdk::queryDirectXCompatModule(riid, intf);
}

extern "C" int WINAPI igwTerminal()
{
    return SDOLTerminal();
}
