#pragma once

#include <map>
#include <mutex>
#include <set>
#include <string>

#include "legacy/interface.h"
#include "SDOA4Client.h"

namespace qtlogin::sdk {

class SdoaAppAdapter final : public ISDOAApp2 {
public:
    static SdoaAppAdapter& instance();

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    STDMETHOD_(int, ShowLoginDialog)(LPLOGINCALLBACKPROC callback, int userData, int reserved) override;
    STDMETHOD_(void, ModifyAppInfo)(const AppInfo* appInfo) override;
    STDMETHOD_(void, Logout)() override;
    STDMETHOD_(int, GetLoginState)(int loginMethod) override;
    STDMETHOD_(void, SetRoleInfo)(const RoleInfo* roleInfo) override;
    STDMETHOD_(int, ShowPaymentDialog)(LPCWSTR src) override;
    STDMETHOD_(int, GetScreenStatus)() override;
    STDMETHOD_(void, SetScreenStatus)(int value) override;
    STDMETHOD_(bool, GetScreenEnabled)() override;
    STDMETHOD_(void, SetScreenEnabled)(bool value) override;
    STDMETHOD_(bool, GetTaskBarPosition)(PPOINT position) override;
    STDMETHOD_(bool, SetTaskBarPosition)(const PPOINT position) override;
    STDMETHOD_(bool, GetFocus)() override;
    STDMETHOD_(void, SetFocus)(bool value) override;
    STDMETHOD_(bool, HasUI)(const PPOINT position) override;
    STDMETHOD_(int, GetTaskBarMode)() override;
    STDMETHOD_(void, SetTaskBarMode)(int barMode) override;
    STDMETHOD_(int, GetSelfCursor)() override;
    STDMETHOD_(void, SetSelfCursor)(int value) override;
    STDMETHOD_(int, OpenWidget)(LPCWSTR widgetNameSpace) override;
    STDMETHOD_(int, OpenWidgetEx)(LPCWSTR widgetNameSpace, LPCWSTR param, int flag) override;
    STDMETHOD_(int, CloseWidget)(LPCWSTR widgetNameSpace) override;
    STDMETHOD_(int, ExecuteWidget)(LPCWSTR widgetNameSpaceOrGuid, LPCWSTR param) override;
    STDMETHOD_(int, WidgetExists)(LPCWSTR widgetNameSpaceOrGuid) override;
    STDMETHOD_(int, OpenWindow)(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode) override;
    STDMETHOD_(int, CloseWindow)(LPCWSTR winName) override;
    STDMETHOD_(int, GetWinProperty)(LPCWSTR winName, SDOAWinProperty* winProperty) override;
    STDMETHOD_(int, SetWinProperty)(LPCWSTR winName, SDOAWinProperty* winProperty) override;
    STDMETHOD_(int, LoginDirect)(LPCSTR sessionId, LPCSTR additional, int reserved) override;
    STDMETHOD_(int, GetClientSignature)(LPCWSTR indication, BSTR* signature) override;

    int handleSdolLoginCallback(int errorCode, const SDOLLoginResult* result, int userData, int reserved);

private:
    SdoaAppAdapter() = default;

    std::wstring safeKey(LPCWSTR value) const;
    int openWidgetLocked(const std::wstring& widgetNameSpace);

    std::mutex mutex_;
    int loginState_ = SDOA_LOGINSTATE_NONE;
    int screenStatus_ = SDOA_SCREEN_NORMAL;
    bool screenEnabled_ = true;
    POINT taskBarPosition_{0, 0};
    bool focus_ = false;
    int taskBarMode_ = SDOA_BARMODE_ICONBOTTOM;
    int selfCursor_ = SDOA_CURSOR_AUTO;
    std::set<std::wstring> widgets_;
    std::map<std::wstring, SDOAWinProperty> windows_;
    std::wstring roleName_;
    int roleSex_ = 0;
    LPLOGINCALLBACKPROC activeLoginCallback_ = nullptr;
};

class SdoaAppUtilsAdapter final : public ISDOAAppUtils {
public:
    static SdoaAppUtilsAdapter& instance();

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    STDMETHOD_(void, SetAudioSetting)(LPGETAUDIOSOUNDVOLUME getAudioSoundVolume,
        LPSETAUDIOSOUNDVOLUME setAudioSoundVolume,
        LPGETAUDIOEFFECTVOLUME getAudioEffectVolume,
        LPSETAUDIOEFFECTVOLUME setAudioEffectVolume) override;
    STDMETHOD_(void, NodifyAudioChanged)() override;
    STDMETHOD_(void, SetCreateChannelCallback)(const LPCREATECHANNEL createChannel) override;

private:
    SdoaAppUtilsAdapter() = default;
};

class SdoaClientServiceAdapter final : public ISDOAClientService {
public:
    static SdoaClientServiceAdapter& instance();

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    STDMETHOD_(int, SetValue)(const BSTR key, const BSTR value) override;
    STDMETHOD_(int, Query)(const BSTR service) override;
    STDMETHOD_(int, AsyncQuery)(const BSTR service, LPQUERYCALLBACK callback, int userData) override;
    STDMETHOD_(int, GetValue)(const BSTR key, BSTR* value) override;

private:
    SdoaClientServiceAdapter() = default;

    std::mutex mutex_;
    std::map<std::wstring, std::wstring> values_;
};

int initializeSdoaRuntime(DWORD sdkVersion, const AppInfo* appInfo);
bool querySdoaModule(REFIID riid, void** intf);

}
