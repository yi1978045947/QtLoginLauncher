#include <mutex>

#include <windows.h>

#include "legacy/interface.h"
#include "logger.h"

namespace {

class UpdateEntryAdapter final : public ISDOLUpdater {
public:
    static UpdateEntryAdapter& instance()
    {
        static UpdateEntryAdapter adapter;
        return adapter;
    }

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override
    {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ISDOLUpdater)) {
            *object = static_cast<ISDOLUpdater*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHOD_(ULONG, AddRef)() override
    {
        return 1;
    }

    STDMETHOD_(ULONG, Release)() override
    {
        return 1;
    }

    STDMETHOD_(int, SetEventCallback)(LPSDOLUPDATEREVENTCALLBACK callback, int userData, int reserved) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = callback;
        userData_ = userData;
        reserved_ = reserved;
        return SDOL_ERRORCODE_OK;
    }

    STDMETHOD_(int, UpdateReport)(const SDOLUpdateReport* report) override
    {
        if (!report || report->Size < sizeof(SDOLUpdateReport)) {
            return SDOL_ERRORCODE_INVALIDPARAM;
        }
        qtlogin::common::logLine(L"update_entry", L"UpdateReport accepted");
        return SDOL_ERRORCODE_OK;
    }

    STDMETHOD_(int, QuitLauncher)() override
    {
        LPSDOLUPDATEREVENTCALLBACK callback = nullptr;
        int userData = 0;
        int reserved = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback = callback_;
            userData = userData_;
            reserved = reserved_;
        }

        if (callback) {
            return callback(L"Quit", userData, reserved);
        }
        return SDOL_ERRORCODE_OK;
    }

private:
    std::mutex mutex_;
    LPSDOLUPDATEREVENTCALLBACK callback_ = nullptr;
    int userData_ = 0;
    int reserved_ = 0;
};

int getUpdaterModule(REFIID riid, void** intf)
{
    if (!intf) {
        return SDOL_ERRORCODE_INVALIDPARAM;
    }
    *intf = nullptr;
    return SUCCEEDED(UpdateEntryAdapter::instance().QueryInterface(riid, intf))
        ? SDOL_ERRORCODE_OK
        : SDOL_ERRORCODE_NOT_SUPPORT;
}

}

extern "C" int WINAPI SDOLInitialize(const SDOLAppInfo*)
{
    qtlogin::common::logLine(L"update_entry", L"SDOLInitialize");
    return SDOL_ERRORCODE_OK;
}

extern "C" int WINAPI SDOLGetModule(REFIID riid, void** intf)
{
    return getUpdaterModule(riid, intf);
}

extern "C" int WINAPI SDOLTerminal()
{
    qtlogin::common::logLine(L"update_entry", L"SDOLTerminal");
    return SDOL_ERRORCODE_OK;
}

extern "C" int WINAPI igwInitialize(const SDOLAppInfo* appInfo)
{
    return SDOLInitialize(appInfo);
}

extern "C" int WINAPI igwGetModule(REFIID riid, void** intf)
{
    return SDOLGetModule(riid, intf);
}

extern "C" int WINAPI igwTerminal()
{
    return SDOLTerminal();
}
