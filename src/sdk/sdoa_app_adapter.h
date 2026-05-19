#pragma once

#include <map>
#include <mutex>
#include <set>
#include <string>

#include "legacy/interface.h"
#include "SDOA4Client.h"

namespace qtlogin::sdk {

class SdoaAppAdapter final : public ISDOAApp4 {
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
    STDMETHOD_(int, LoginFeedback)(LPCWSTR sessionId, int result, int errorCode) override;
    STDMETHOD_(int, OpenMatrixGamePay)() override;
    STDMETHOD_(int, SetLoginDialogState)(int state) override;
    STDMETHOD_(int, OpenActivityWindow)(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode) override;
    STDMETHOD_(int, AuthCodeLogin)(LPLOGINAUTHCODECALLBACKPROC callback, LPCSTR authCode) override;
    STDMETHOD_(int, GetTicket)(BSTR* ticket, BSTR* sndaid) override;
    STDMETHOD_(int, OpenXinYouLoginIeAgain)() override;
    STDMETHOD_(int, OpenXinIeWindow)(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode) override;
    STDMETHOD(OnGameProc)(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) override;
    STDMETHOD_(void*, OnGetSharedImage)() override;
    STDMETHOD_(int, GetGameResolution)(GameSetResolution* resolution) override;
    STDMETHOD_(int, RegisterPayEvent)(LPDRAGONLENOVOPAYWINDOWCLOSEDCALLBACKPROC callback) override;
    STDMETHOD_(int, ShowDragonLenovoLoginDlg)(LPLOGINCALLBACKPROC callback, int userData, int reserved) override;
    STDMETHOD_(int, AsyncShowDragonLenovoPayWindow)(LPCWSTR orderId, LPCWSTR productId, LPCWSTR groupId, LPCWSTR areaId, LPCWSTR roleId, LPCWSTR extend) override;
    STDMETHOD_(int, GetTicketForAppid)(BSTR* ticket, BSTR* sndaid, int appId) override;
    STDMETHOD_(int, VertifyDoubleLogin)(int isDoubleLogin) override;
    STDMETHOD_(int, SetDoubleLoginCallBack)(LPDOUBLELOGINCALLBACKPROC callback) override;
    STDMETHOD_(int, OpenFaceVertifyDlg)(LPFACEVERTIFYCALLBACKPROC callback, LPCWSTR verifyType) override;
    STDMETHOD_(int, OpenCollectUserMsgDlg)(LPCOLLECTUSERMSGCALLBACKPROC callback, LPCWSTR collectMsgType) override;
    STDMETHOD_(int, SetLoginMode)(int loginMode) override;
    STDMETHOD_(int, GetAccountLoginState)(LPLOGINSTATCALLBACKPROC callback) override;
    STDMETHOD_(int, SetOwnerWindow)(HWND hwnd) override;
    STDMETHOD_(int, MoveLoginDialog)(int x, int y) override;
    STDMETHOD_(int, SessionLoginGame)(LPLOGINGAMECALLBACKPROC callback, const AppInfo* appInfo) override;
    STDMETHOD_(int, SetDpiSetting)(int dpi) override;
    STDMETHOD_(int, GhomePay)(const char* extend, LPLOGINPAYCALLBACKPROC callback) override;
    STDMETHOD_(int, GhomeGetCPSChannelInfo)(LPLOGINGETCHANNELCODECALLBACKPROC callback) override;

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
    int dpiSetting_ = 96;
    bool doubleLoginEnabled_ = false;
    LPDOUBLELOGINCALLBACKPROC doubleLoginCallback_ = nullptr;
    int taskBarMode_ = SDOA_BARMODE_ICONBOTTOM;
    int selfCursor_ = SDOA_CURSOR_AUTO;
    std::set<std::wstring> widgets_;
    std::map<std::wstring, SDOAWinProperty> windows_;
    std::wstring roleName_;
    int roleSex_ = 0;
    LPLOGINCALLBACKPROC activeLoginCallback_ = nullptr;
};

class SdoaApp5Adapter final : public ISDOAApp5 {
public:
    static SdoaApp5Adapter& instance();

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    STDMETHOD_(int, OpenFaceVerifyDialog)(LPFACEVERTIFYCALLBACKPROC callback, LPCWSTR verifyType) override;
    STDMETHOD_(int, OpenCollectUserMsgDialog)(LPCOLLECTUSERMSGCALLBACKPROC callback, LPCWSTR collectMsgType) override;

private:
    SdoaApp5Adapter() = default;
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
