#include "sdoa_app_adapter.h"

#include <oleauto.h>
#include <sstream>

#include "legacy/interface.h"
#include "sdk_runtime.h"
#include "string_util.h"

namespace qtlogin::sdk {
namespace {

int mapSdolErrorToSdoaError(int errorCode)
{
    switch (errorCode) {
    case SDOL_ERRORCODE_OK:
        return SDOA_ERRORCODE_OK;
    case SDOL_ERRORCODE_LOGINCANCEL:
        return SDOA_ERRORCODE_CANCEL;
    case SDOL_ERRORCODE_INVALIDPARAM:
        return SDOA_ERRORCODE_INVALIDPARAM;
    case SDOL_ERRORCODE_UNEXCEPT:
        return SDOA_ERRORCODE_UNEXCEPT;
    default:
        return errorCode == SDOL_ERRORCODE_FAILED ? SDOA_ERRORCODE_FAILED : SDOA_ERRORCODE_UNKNOWN;
    }
}

SDOLAppInfo toSdolAppInfo(const AppInfo* appInfo)
{
    SDOLAppInfo result{};
    result.Size = sizeof(result);
    result.AppID = appInfo ? appInfo->nAppID : 0;
    result.AppName = appInfo ? appInfo->pwcsAppName : nullptr;
    result.AppVer = appInfo ? appInfo->pwcsAppVer : nullptr;
    result.AreaId = appInfo ? appInfo->nAreaId : -1;
    result.GroupId = appInfo ? appInfo->nGroupId : -1;
    return result;
}

std::wstring bstrToWide(const BSTR value)
{
    return value ? std::wstring(value, SysStringLen(value)) : std::wstring();
}

std::string safeUtf8(const std::wstring& value)
{
    return value.empty() ? std::string() : common::wideToUtf8(value);
}

std::mutex g_loginCallbackMutex;
SdoaAppAdapter* g_loginCallbackAdapter = nullptr;

int CALLBACK sdoaLoginThunk(int errorCode, const SDOLLoginResult* result, int userData, int reserved)
{
    SdoaAppAdapter* adapter = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_loginCallbackMutex);
        adapter = g_loginCallbackAdapter;
    }
    return adapter ? adapter->handleSdolLoginCallback(errorCode, result, userData, reserved)
                   : SDOL_LOGINRESULT_CLOSE;
}

}

SdoaAppAdapter& SdoaAppAdapter::instance()
{
    static SdoaAppAdapter adapter;
    return adapter;
}

HRESULT SdoaAppAdapter::QueryInterface(REFIID riid, void** object)
{
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;
    if (riid == IID_IUnknown || riid == __uuidof(ISDOAApp)) {
        *object = static_cast<ISDOAApp*>(this);
    } else if (riid == __uuidof(ISDOAApp2)) {
        *object = static_cast<ISDOAApp2*>(this);
    } else if (riid == __uuidof(ISDOAApp3)) {
        *object = static_cast<ISDOAApp3*>(this);
    } else if (riid == __uuidof(ISDOAApp4)) {
        *object = static_cast<ISDOAApp4*>(this);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG SdoaAppAdapter::AddRef()
{
    return 1;
}

ULONG SdoaAppAdapter::Release()
{
    return 1;
}

int SdoaAppAdapter::ShowLoginDialog(LPLOGINCALLBACKPROC callback, int userData, int reserved)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loginState_ = SDOA_LOGINSTATE_CONNECTING;
        activeLoginCallback_ = callback;
    }
    {
        std::lock_guard<std::mutex> lock(g_loginCallbackMutex);
        g_loginCallbackAdapter = this;
    }

    const int result = SdkRuntime::instance().showLoginDialog(&sdoaLoginThunk, userData, reserved);

    {
        std::lock_guard<std::mutex> lock(g_loginCallbackMutex);
        if (g_loginCallbackAdapter == this) {
            g_loginCallbackAdapter = nullptr;
        }
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        activeLoginCallback_ = nullptr;
        if (result != SDOL_ERRORCODE_OK && result != SDOL_ERRORCODE_LOGINCANCEL && loginState_ == SDOA_LOGINSTATE_CONNECTING) {
            loginState_ = SDOA_LOGINSTATE_FAILED;
        }
    }
    return mapSdolErrorToSdoaError(result);
}

void SdoaAppAdapter::ModifyAppInfo(const AppInfo* appInfo)
{
    const SDOLAppInfo sdolAppInfo = toSdolAppInfo(appInfo);
    SdkRuntime::instance().modifyAppInfo(&sdolAppInfo);
}

void SdoaAppAdapter::Logout()
{
    SdkRuntime::instance().logout();
    std::lock_guard<std::mutex> lock(mutex_);
    loginState_ = SDOA_LOGINSTATE_LOGOUT;
}

int SdoaAppAdapter::GetLoginState(int)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return loginState_;
}

void SdoaAppAdapter::SetRoleInfo(const RoleInfo* roleInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    roleName_ = roleInfo && roleInfo->pwcsRoleName ? roleInfo->pwcsRoleName : L"";
    roleSex_ = roleInfo ? roleInfo->nSex : 0;
}

int SdoaAppAdapter::ShowPaymentDialog(LPCWSTR src)
{
    return SdkRuntime::instance().openWindow(L"payment", L"Payment", src, 0, 0, 760, 560, L"center");
}

int SdoaAppAdapter::GetScreenStatus()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return screenStatus_;
}

void SdoaAppAdapter::SetScreenStatus(int value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    screenStatus_ = value;
}

bool SdoaAppAdapter::GetScreenEnabled()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return screenEnabled_;
}

void SdoaAppAdapter::SetScreenEnabled(bool value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    screenEnabled_ = value;
}

bool SdoaAppAdapter::GetTaskBarPosition(PPOINT position)
{
    if (!position) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    *position = taskBarPosition_;
    return true;
}

bool SdoaAppAdapter::SetTaskBarPosition(const PPOINT position)
{
    if (!position) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    taskBarPosition_ = *position;
    return true;
}

bool SdoaAppAdapter::GetFocus()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return focus_;
}

void SdoaAppAdapter::SetFocus(bool value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    focus_ = value;
}

bool SdoaAppAdapter::HasUI(const PPOINT)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return !windows_.empty() || !widgets_.empty();
}

int SdoaAppAdapter::GetTaskBarMode()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return taskBarMode_;
}

void SdoaAppAdapter::SetTaskBarMode(int barMode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    taskBarMode_ = barMode;
}

int SdoaAppAdapter::GetSelfCursor()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return selfCursor_;
}

void SdoaAppAdapter::SetSelfCursor(int value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    selfCursor_ = value;
}

std::wstring SdoaAppAdapter::safeKey(LPCWSTR value) const
{
    return value ? std::wstring(value) : std::wstring();
}

int SdoaAppAdapter::openWidgetLocked(const std::wstring& widgetNameSpace)
{
    if (widgetNameSpace.empty()) {
        return SDOA_ERRORCODE_INVALIDPARAM;
    }
    widgets_.insert(widgetNameSpace);
    return SDOA_OK;
}

int SdoaAppAdapter::OpenWidget(LPCWSTR widgetNameSpace)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return openWidgetLocked(safeKey(widgetNameSpace));
}

int SdoaAppAdapter::OpenWidgetEx(LPCWSTR widgetNameSpace, LPCWSTR, int)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return openWidgetLocked(safeKey(widgetNameSpace));
}

int SdoaAppAdapter::CloseWidget(LPCWSTR widgetNameSpace)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const std::wstring key = safeKey(widgetNameSpace);
    if (key.empty()) {
        return SDOA_ERRORCODE_INVALIDPARAM;
    }
    widgets_.erase(key);
    return SDOA_OK;
}

int SdoaAppAdapter::ExecuteWidget(LPCWSTR widgetNameSpaceOrGuid, LPCWSTR)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return widgets_.count(safeKey(widgetNameSpaceOrGuid)) ? SDOA_OK : SDOA_FALSE;
}

int SdoaAppAdapter::WidgetExists(LPCWSTR widgetNameSpaceOrGuid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return widgets_.count(safeKey(widgetNameSpaceOrGuid)) ? 1 : 0;
}

int SdoaAppAdapter::OpenWindow(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode)
{
    const int result = SdkRuntime::instance().openWindow(winType, winName, src, left, top, width, height, mode);
    if (result == SDOL_ERRORCODE_OK) {
        std::lock_guard<std::mutex> lock(mutex_);
        const std::wstring key = safeKey(winName).empty() ? safeKey(src) : safeKey(winName);
        windows_[key] = SDOAWinProperty{SDOA_WINDOWSTYPE_WINDOWS, SDOA_WINDOWSSTATUS_SHOW, left, top, width, height};
    }
    return mapSdolErrorToSdoaError(result);
}

int SdoaAppAdapter::CloseWindow(LPCWSTR winName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    windows_.erase(safeKey(winName));
    return SDOA_OK;
}

int SdoaAppAdapter::GetWinProperty(LPCWSTR winName, SDOAWinProperty* winProperty)
{
    if (!winProperty) {
        return SDOA_ERRORCODE_INVALIDPARAM;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = windows_.find(safeKey(winName));
    if (it == windows_.end()) {
        return SDOA_FALSE;
    }
    *winProperty = it->second;
    return SDOA_OK;
}

int SdoaAppAdapter::SetWinProperty(LPCWSTR winName, SDOAWinProperty* winProperty)
{
    if (!winProperty) {
        return SDOA_ERRORCODE_INVALIDPARAM;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    windows_[safeKey(winName)] = *winProperty;
    return SDOA_OK;
}

int SdoaAppAdapter::LoginDirect(LPCSTR sessionId, LPCSTR additional, int)
{
    const std::wstring session = common::utf8ToWide(sessionId ? sessionId : "");
    const std::wstring appendix = common::utf8ToWide(additional ? additional : "");
    SdkRuntime::instance().recordLoginResult(session, L"", L"1", appendix, session);
    std::lock_guard<std::mutex> lock(mutex_);
    loginState_ = SDOA_LOGINSTATE_OK;
    return SDOA_OK;
}

int SdoaAppAdapter::GetClientSignature(LPCWSTR indication, BSTR* signature)
{
    if (!signature) {
        return SDOA_ERRORCODE_INVALIDPARAM;
    }
    std::wstringstream stream;
    stream << L"qtlogin-signature:";
    stream << (indication ? indication : L"");
    *signature = SysAllocString(stream.str().c_str());
    return *signature ? SDOA_OK : SDOA_ERRORCODE_FAILED;
}

int SdoaAppAdapter::LoginFeedback(LPCWSTR, int, int)
{
    return SDOA_OK;
}

int SdoaAppAdapter::OpenMatrixGamePay()
{
    return OpenWindow(L"HTML", L"MatrixGamePay", L"https://www.daoyu8.com/#/", 0, 0, 900, 640, L"center");
}

int SdoaAppAdapter::SetLoginDialogState(int)
{
    return SDOA_OK;
}

int SdoaAppAdapter::OpenActivityWindow(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode)
{
    return OpenWindow(winType, winName, src, left, top, width, height, mode);
}

int SdoaAppAdapter::AuthCodeLogin(LPLOGINAUTHCODECALLBACKPROC callback, LPCSTR)
{
    if (callback) {
        callback(SDOA_ERRORCODE_FAILED, nullptr);
    }
    return SDOA_ERRORCODE_FAILED;
}

int SdoaAppAdapter::GetTicket(BSTR* ticket, BSTR* sndaid)
{
    return SdkRuntime::instance().getTicket(ticket, sndaid);
}

int SdoaAppAdapter::OpenXinYouLoginIeAgain()
{
    return SDOA_ERRORCODE_FAILED;
}

int SdoaAppAdapter::OpenXinIeWindow(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode)
{
    return OpenWindow(winType, winName, src, left, top, width, height, mode);
}

HRESULT SdoaAppAdapter::OnGameProc(HWND, UINT, WPARAM, LPARAM, LRESULT* result)
{
    if (result) {
        *result = 0;
    }
    return S_FALSE;
}

void* SdoaAppAdapter::OnGetSharedImage()
{
    return nullptr;
}

int SdoaAppAdapter::GetGameResolution(GameSetResolution* resolution)
{
    if (!resolution) {
        return SDOA_ERRORCODE_INVALIDPARAM;
    }
    resolution->full_screen = false;
    resolution->width = 0;
    resolution->height = 0;
    return SDOA_OK;
}

int SdoaAppAdapter::RegisterPayEvent(LPDRAGONLENOVOPAYWINDOWCLOSEDCALLBACKPROC)
{
    return SDOA_OK;
}

int SdoaAppAdapter::ShowDragonLenovoLoginDlg(LPLOGINCALLBACKPROC callback, int userData, int reserved)
{
    return ShowLoginDialog(callback, userData, reserved);
}

int SdoaAppAdapter::AsyncShowDragonLenovoPayWindow(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR)
{
    return SDOA_ERRORCODE_FAILED;
}

int SdoaAppAdapter::GetTicketForAppid(BSTR* ticket, BSTR* sndaid, int)
{
    return GetTicket(ticket, sndaid);
}

int SdoaAppAdapter::VertifyDoubleLogin(int isDoubleLogin)
{
    std::lock_guard<std::mutex> lock(mutex_);
    doubleLoginEnabled_ = isDoubleLogin != 0;
    return SDOA_OK;
}

int SdoaAppAdapter::SetDoubleLoginCallBack(LPDOUBLELOGINCALLBACKPROC callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    doubleLoginCallback_ = callback;
    return SDOA_OK;
}

int SdoaAppAdapter::OpenFaceVertifyDlg(LPFACEVERTIFYCALLBACKPROC callback, LPCWSTR verifyType)
{
    return SdkRuntime::instance().openFaceVerifyDialog(callback, verifyType);
}

int SdoaAppAdapter::OpenCollectUserMsgDlg(LPCOLLECTUSERMSGCALLBACKPROC callback, LPCWSTR collectMsgType)
{
    return SdkRuntime::instance().openCollectUserMsgDialog(callback, collectMsgType);
}

int SdoaAppAdapter::SetLoginMode(int loginMode)
{
    return SdkRuntime::instance().setLoginMode(loginMode);
}

int SdoaAppAdapter::GetAccountLoginState(LPLOGINSTATCALLBACKPROC callback)
{
    int state = SDOA_LOGINSTATE_NONE;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state = loginState_;
    }
    if (callback) {
        callback(state == SDOA_LOGINSTATE_OK ? SDOA_ERRORCODE_OK : SDOA_ERRORCODE_FAILED);
    }
    return SDOA_OK;
}

int SdoaAppAdapter::SetOwnerWindow(HWND hwnd)
{
    return SdkRuntime::instance().setOwnerWindow(hwnd);
}

int SdoaAppAdapter::MoveLoginDialog(int x, int y)
{
    return SdkRuntime::instance().moveLoginDialog(x, y);
}

int SdoaAppAdapter::SessionLoginGame(LPLOGINGAMECALLBACKPROC callback, const AppInfo*)
{
    if (callback) {
        callback(SDOA_ERRORCODE_FAILED, nullptr);
    }
    return SDOA_ERRORCODE_FAILED;
}

int SdoaAppAdapter::SetDpiSetting(int dpi)
{
    std::lock_guard<std::mutex> lock(mutex_);
    dpiSetting_ = dpi > 0 ? dpi : 96;
    return SDOA_OK;
}

int SdoaAppAdapter::GhomePay(const char* extend, LPLOGINPAYCALLBACKPROC callback)
{
    return SdkRuntime::instance().ghomePay(extend, callback);
}

int SdoaAppAdapter::GhomeGetCPSChannelInfo(LPLOGINGETCHANNELCODECALLBACKPROC callback)
{
    GhomeChannelInfo info{};
    info.szApplicationChannel = "qtlogin";
    info.szChannelCode = "qtlogin-demo";
    info.szAdtraceId = "";
    if (callback) {
        callback(SDOA_ERRORCODE_OK, &info);
    }
    return SDOA_OK;
}

int SdoaAppAdapter::handleSdolLoginCallback(int errorCode, const SDOLLoginResult* result, int userData, int)
{
    LPLOGINCALLBACKPROC callback = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = activeLoginCallback_;
        if (errorCode == SDOL_ERRORCODE_OK && result) {
            loginState_ = SDOA_LOGINSTATE_OK;
        } else if (errorCode == SDOL_ERRORCODE_LOGINCANCEL) {
            loginState_ = SDOA_LOGINSTATE_NONE;
        } else {
            loginState_ = SDOA_LOGINSTATE_FAILED;
        }
    }
    if (!callback) {
        return SDOL_LOGINRESULT_CLOSE;
    }

    std::string sessionId;
    std::string sndaid;
    std::string identityState;
    std::string appendix;
    if (result) {
        sessionId = safeUtf8(result->SessionId ? result->SessionId : L"");
        sndaid = safeUtf8(result->Sndaid ? result->Sndaid : L"");
        identityState = safeUtf8(result->IdentityState ? result->IdentityState : L"");
        appendix = safeUtf8(result->Appendix ? result->Appendix : L"");
    }

    LoginResult sdoaResult{};
    sdoaResult.cbSize = sizeof(sdoaResult);
    sdoaResult.szSessionId = sessionId.c_str();
    sdoaResult.szSndaid = sndaid.c_str();
    sdoaResult.szIdentityState = identityState.c_str();
    sdoaResult.szAppendix = appendix.c_str();
    sdoaResult.szAdditional = appendix.c_str();

    const int sdoaError = mapSdolErrorToSdoaError(errorCode);
    const BOOL shouldClose = callback(sdoaError, sdoaError == SDOA_ERRORCODE_OK ? &sdoaResult : nullptr, userData, 0);
    return shouldClose ? SDOL_LOGINRESULT_CLOSE : SDOL_LOGINRESULT_KEEP_SHOWN;
}

SdoaApp5Adapter& SdoaApp5Adapter::instance()
{
    static SdoaApp5Adapter adapter;
    return adapter;
}

HRESULT SdoaApp5Adapter::QueryInterface(REFIID riid, void** object)
{
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;
    if (riid == IID_IUnknown || riid == __uuidof(ISDOAApp5)) {
        *object = static_cast<ISDOAApp5*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG SdoaApp5Adapter::AddRef() { return 1; }
ULONG SdoaApp5Adapter::Release() { return 1; }

int SdoaApp5Adapter::OpenFaceVerifyDialog(LPFACEVERTIFYCALLBACKPROC callback, LPCWSTR verifyType)
{
    return SdkRuntime::instance().openFaceVerifyDialog(callback, verifyType);
}

int SdoaApp5Adapter::OpenCollectUserMsgDialog(LPCOLLECTUSERMSGCALLBACKPROC callback, LPCWSTR collectMsgType)
{
    return SdkRuntime::instance().openCollectUserMsgDialog(callback, collectMsgType);
}

SdoaAppUtilsAdapter& SdoaAppUtilsAdapter::instance()
{
    static SdoaAppUtilsAdapter adapter;
    return adapter;
}

HRESULT SdoaAppUtilsAdapter::QueryInterface(REFIID riid, void** object)
{
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;
    if (riid == IID_IUnknown || riid == __uuidof(ISDOAAppUtils)) {
        *object = static_cast<ISDOAAppUtils*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG SdoaAppUtilsAdapter::AddRef() { return 1; }
ULONG SdoaAppUtilsAdapter::Release() { return 1; }
void SdoaAppUtilsAdapter::SetAudioSetting(LPGETAUDIOSOUNDVOLUME, LPSETAUDIOSOUNDVOLUME, LPGETAUDIOEFFECTVOLUME, LPSETAUDIOEFFECTVOLUME) {}
void SdoaAppUtilsAdapter::NodifyAudioChanged() {}
void SdoaAppUtilsAdapter::SetCreateChannelCallback(const LPCREATECHANNEL) {}

SdoaClientServiceAdapter& SdoaClientServiceAdapter::instance()
{
    static SdoaClientServiceAdapter adapter;
    return adapter;
}

HRESULT SdoaClientServiceAdapter::QueryInterface(REFIID riid, void** object)
{
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;
    if (riid == IID_IUnknown || riid == __uuidof(ISDOAClientService)) {
        *object = static_cast<ISDOAClientService*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG SdoaClientServiceAdapter::AddRef() { return 1; }
ULONG SdoaClientServiceAdapter::Release() { return 1; }

int SdoaClientServiceAdapter::SetValue(const BSTR key, const BSTR value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    values_[bstrToWide(key)] = bstrToWide(value);
    return SDOA_OK;
}

int SdoaClientServiceAdapter::Query(const BSTR)
{
    return SDOA_OK;
}

int SdoaClientServiceAdapter::AsyncQuery(const BSTR, LPQUERYCALLBACK callback, int userData)
{
    if (callback) {
        callback(SDOA_OK, userData);
    }
    return SDOA_OK;
}

int SdoaClientServiceAdapter::GetValue(const BSTR key, BSTR* value)
{
    if (!value) {
        return SDOA_ERRORCODE_INVALIDPARAM;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = values_.find(bstrToWide(key));
    *value = SysAllocString(it == values_.end() ? L"" : it->second.c_str());
    return *value ? SDOA_OK : SDOA_ERRORCODE_FAILED;
}

int initializeSdoaRuntime(DWORD sdkVersion, const AppInfo* appInfo)
{
    if (sdkVersion != SDOA_SDK_VERSION || !appInfo || appInfo->cbSize < sizeof(AppInfo)) {
        return SDOA_ERRORCODE_INVALIDPARAM;
    }

    const SDOLAppInfo sdolAppInfo = toSdolAppInfo(appInfo);
    return mapSdolErrorToSdoaError(SdkRuntime::instance().initialize(&sdolAppInfo));
}

bool querySdoaModule(REFIID riid, void** intf)
{
    if (!intf) {
        return false;
    }
    *intf = nullptr;
    if (SUCCEEDED(SdoaAppAdapter::instance().QueryInterface(riid, intf))) {
        return true;
    }
    if (SUCCEEDED(SdoaApp5Adapter::instance().QueryInterface(riid, intf))) {
        return true;
    }
    if (SUCCEEDED(SdoaAppUtilsAdapter::instance().QueryInterface(riid, intf))) {
        return true;
    }
    if (SUCCEEDED(SdoaClientServiceAdapter::instance().QueryInterface(riid, intf))) {
        return true;
    }
    return false;
}

}
