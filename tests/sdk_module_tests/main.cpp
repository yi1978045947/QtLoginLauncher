#include <iostream>

#include <windows.h>

#include "legacy/interface.h"

struct AppInfoAbiProbe {
    DWORD cbSize;
    int nAppID;
    LPCWSTR pwcsAppName;
    LPCWSTR pwcsAppVer;
    int nRenderType;
    int nMaxUser;
    int nAreaId;
    int nGroupId;
};

struct LoginResultAbiProbe {
    DWORD cbSize;
    LPCSTR szSessionId;
    LPCSTR szSndaid;
    LPCSTR szIdentityState;
    LPCSTR szAppendix;
    LPCSTR szAdditional;
};

struct SDOAWinPropertyAbiProbe {
    int nWinType;
    int nStatus;
    int nLeft;
    int nTop;
    int nWidth;
    int nHeight;
};

typedef BOOL(CALLBACK* LPLOGINCALLBACKPROBE)(int, const LoginResultAbiProbe*, int, int);

struct __declspec(uuid("C8187484-1C5F-48DF-8497-2136A72A5733")) ISDOAAppAbiProbe : IUnknown {
    virtual int STDMETHODCALLTYPE ShowLoginDialog(LPLOGINCALLBACKPROBE callback, int userData, int reserved) = 0;
    virtual void STDMETHODCALLTYPE ModifyAppInfo(const AppInfoAbiProbe* appInfo) = 0;
    virtual void STDMETHODCALLTYPE Logout() = 0;
    virtual int STDMETHODCALLTYPE GetLoginState(int loginMethod) = 0;
    virtual void STDMETHODCALLTYPE SetRoleInfo(const void* roleInfo) = 0;
    virtual int STDMETHODCALLTYPE ShowPaymentDialog(LPCWSTR src) = 0;
    virtual int STDMETHODCALLTYPE GetScreenStatus() = 0;
    virtual void STDMETHODCALLTYPE SetScreenStatus(int value) = 0;
    virtual bool STDMETHODCALLTYPE GetScreenEnabled() = 0;
    virtual void STDMETHODCALLTYPE SetScreenEnabled(bool value) = 0;
    virtual bool STDMETHODCALLTYPE GetTaskBarPosition(PPOINT position) = 0;
    virtual bool STDMETHODCALLTYPE SetTaskBarPosition(const PPOINT position) = 0;
    virtual bool STDMETHODCALLTYPE GetFocus() = 0;
    virtual void STDMETHODCALLTYPE SetFocus(bool value) = 0;
    virtual bool STDMETHODCALLTYPE HasUI(const PPOINT position) = 0;
    virtual int STDMETHODCALLTYPE GetTaskBarMode() = 0;
    virtual void STDMETHODCALLTYPE SetTaskBarMode(int barMode) = 0;
    virtual int STDMETHODCALLTYPE GetSelfCursor() = 0;
    virtual void STDMETHODCALLTYPE SetSelfCursor(int value) = 0;
    virtual int STDMETHODCALLTYPE OpenWidget(LPCWSTR widgetNameSpace) = 0;
    virtual int STDMETHODCALLTYPE OpenWidgetEx(LPCWSTR widgetNameSpace, LPCWSTR param, int flag) = 0;
    virtual int STDMETHODCALLTYPE CloseWidget(LPCWSTR widgetNameSpace) = 0;
    virtual int STDMETHODCALLTYPE ExecuteWidget(LPCWSTR widgetNameSpaceOrGuid, LPCWSTR param) = 0;
    virtual int STDMETHODCALLTYPE WidgetExists(LPCWSTR widgetNameSpaceOrGuid) = 0;
    virtual int STDMETHODCALLTYPE OpenWindow(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode) = 0;
    virtual int STDMETHODCALLTYPE CloseWindow(LPCWSTR winName) = 0;
    virtual int STDMETHODCALLTYPE GetWinProperty(LPCWSTR winName, SDOAWinPropertyAbiProbe* winProperty) = 0;
    virtual int STDMETHODCALLTYPE SetWinProperty(LPCWSTR winName, SDOAWinPropertyAbiProbe* winProperty) = 0;
};

struct __declspec(uuid("3F35136C-7061-4E69-BF3A-CC78D026F48F")) ISDOAApp2AbiProbe : ISDOAAppAbiProbe {
    virtual int STDMETHODCALLTYPE LoginDirect(LPCSTR sessionId, LPCSTR additional, int reserved) = 0;
    virtual int STDMETHODCALLTYPE GetClientSignature(LPCWSTR indication, BSTR* signature) = 0;
};

typedef BOOL(CALLBACK* LPLOGINSTATCALLBACKPROBE)(int);
typedef BOOL(CALLBACK* LPLOGINGAMECALLBACKPROBE)(int, const LoginResultAbiProbe*);
typedef BOOL(CALLBACK* LPLOGINAUTHCODECALLBACKPROBE)(int, LPCSTR);
typedef BOOL(CALLBACK* LPDOUBLELOGINCALLBACKPROBE)(int, const void*, int, int);
typedef BOOL(CALLBACK* LPFACEVERTIFYCALLBACKPROBE)(int, LPCSTR);
typedef BOOL(CALLBACK* LPCOLLECTUSERMSGCALLBACKPROBE)(int, LPCSTR);
typedef BOOL(CALLBACK* LPLOGINPAYCALLBACKPROBE)(int, const char*);

struct GhomeChannelInfoAbiProbe {
    LPCSTR szApplicationChannel;
    LPCSTR szChannelCode;
    LPCSTR szAdtraceId;
};

typedef BOOL(CALLBACK* LPLOGINGETCHANNELCODECALLBACKPROBE)(int, const GhomeChannelInfoAbiProbe*);

struct GameSetResolutionAbiProbe {
    bool full_screen;
    int width;
    int height;
};

struct __declspec(uuid("7814A3D2-3625-4585-8CA7-072CA295ECE7")) ISDOAApp3AbiProbe : ISDOAApp2AbiProbe {
    virtual int STDMETHODCALLTYPE LoginFeedback(LPCWSTR sessionId, int result, int errorCode) = 0;
    virtual int STDMETHODCALLTYPE OpenMatrixGamePay() = 0;
    virtual int STDMETHODCALLTYPE SetLoginDialogState(int state) = 0;
    virtual int STDMETHODCALLTYPE OpenActivityWindow(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode) = 0;
    virtual int STDMETHODCALLTYPE AuthCodeLogin(LPLOGINAUTHCODECALLBACKPROBE callback, LPCSTR authCode) = 0;
    virtual int STDMETHODCALLTYPE GetTicket(BSTR* ticket, BSTR* sndaid) = 0;
    virtual int STDMETHODCALLTYPE OpenXinYouLoginIeAgain() = 0;
    virtual int STDMETHODCALLTYPE OpenXinIeWindow(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode) = 0;
    virtual HRESULT STDMETHODCALLTYPE OnGameProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) = 0;
    virtual void* STDMETHODCALLTYPE OnGetSharedImage() = 0;
    virtual int STDMETHODCALLTYPE GetGameResolution(GameSetResolutionAbiProbe* resolution) = 0;
    virtual int STDMETHODCALLTYPE RegisterPayEvent(void* callback) = 0;
    virtual int STDMETHODCALLTYPE ShowDragonLenovoLoginDlg(LPLOGINCALLBACKPROBE callback, int userData, int reserved) = 0;
    virtual int STDMETHODCALLTYPE AsyncShowDragonLenovoPayWindow(LPCWSTR orderId, LPCWSTR productId, LPCWSTR groupId, LPCWSTR areaId, LPCWSTR roleId, LPCWSTR extend) = 0;
    virtual int STDMETHODCALLTYPE GetTicketForAppid(BSTR* ticket, BSTR* sndaid, int appId) = 0;
    virtual int STDMETHODCALLTYPE VertifyDoubleLogin(int isDoubleLogin) = 0;
    virtual int STDMETHODCALLTYPE SetDoubleLoginCallBack(LPDOUBLELOGINCALLBACKPROBE callback) = 0;
    virtual int STDMETHODCALLTYPE OpenFaceVertifyDlg(LPFACEVERTIFYCALLBACKPROBE callback, LPCWSTR verifyType) = 0;
    virtual int STDMETHODCALLTYPE OpenCollectUserMsgDlg(LPCOLLECTUSERMSGCALLBACKPROBE callback, LPCWSTR collectMsgType) = 0;
};

struct __declspec(uuid("15B0B068-A577-4695-A1C9-16F87F765A07")) ISDOAApp4AbiProbe : ISDOAApp3AbiProbe {
    virtual int STDMETHODCALLTYPE SetLoginMode(int loginMode) = 0;
    virtual int STDMETHODCALLTYPE GetAccountLoginState(LPLOGINSTATCALLBACKPROBE callback) = 0;
    virtual int STDMETHODCALLTYPE SetOwnerWindow(HWND hwnd) = 0;
    virtual int STDMETHODCALLTYPE MoveLoginDialog(int x, int y) = 0;
    virtual int STDMETHODCALLTYPE SessionLoginGame(LPLOGINGAMECALLBACKPROBE callback, const AppInfoAbiProbe* appInfo) = 0;
    virtual int STDMETHODCALLTYPE SetDpiSetting(int dpi) = 0;
    virtual int STDMETHODCALLTYPE GhomePay(const char* extend, LPLOGINPAYCALLBACKPROBE callback) = 0;
    virtual int STDMETHODCALLTYPE GhomeGetCPSChannelInfo(LPLOGINGETCHANNELCODECALLBACKPROBE callback) = 0;
};

struct __declspec(uuid("CD997351-C50F-4602-995E-E8B3DD890EB8")) ISDOAApp5AbiProbe : IUnknown {
    virtual int STDMETHODCALLTYPE OpenFaceVerifyDialog(LPFACEVERTIFYCALLBACKPROBE callback, LPCWSTR verifyType) = 0;
    virtual int STDMETHODCALLTYPE OpenCollectUserMsgDialog(LPCOLLECTUSERMSGCALLBACKPROBE callback, LPCWSTR collectMsgType) = 0;
};

struct __declspec(uuid("6469AE4A-4862-4248-913C-704179AFE43E")) ISDOADx8AbiProbe : IUnknown {};
struct __declspec(uuid("59B789AB-BEE9-4C68-AA23-BCA9AB1A3E50")) ISDOADx9AbiProbe : IUnknown {};
struct __declspec(uuid("7344D794-2D10-541C-F245-824F5A094B37")) ISDOADx11AbiProbe : IUnknown {};

typedef int(WINAPI* LPigwInitializeProbe)(DWORD sdkVersion, const AppInfoAbiProbe* appInfo);
typedef bool(WINAPI* LPigwGetModuleProbe)(REFIID riid, void** intf);
typedef int(WINAPI* LPigwTerminalProbe)();

namespace {

bool queryModule(LPSDOLGetModule getModule, REFIID riid, const wchar_t* name)
{
    IUnknown* module = nullptr;
    const int code = getModule(riid, reinterpret_cast<void**>(&module));
    if (code != SDOL_ERRORCODE_OK || !module) {
        std::wcerr << L"SDOLGetModule(" << name << L") failed, code=" << code << L"\n";
        return false;
    }
    module->Release();
    return true;
}

bool queryIgwModule(LPigwGetModuleProbe getModule, REFIID riid, const wchar_t* name)
{
    IUnknown* module = nullptr;
    if (!getModule(riid, reinterpret_cast<void**>(&module)) || !module) {
        std::wcerr << L"igwGetModule(" << name << L") failed\n";
        return false;
    }
    module->Release();
    return true;
}

bool g_payCallbackCalled = false;
int g_payCallbackCode = SDOL_ERRORCODE_FAILED;
bool g_loginCallbackCalled = false;
int g_loginCallbackCode = SDOL_ERRORCODE_FAILED;
std::wstring g_loginCallbackSession;
bool g_channelCallbackCalled = false;
int g_channelCallbackCode = SDOL_ERRORCODE_FAILED;
bool g_faceCallbackCalled = false;
int g_faceCallbackCode = SDOL_ERRORCODE_FAILED;
bool g_collectCallbackCalled = false;
int g_collectCallbackCode = SDOL_ERRORCODE_FAILED;

BOOL CALLBACK payCallback(int errorCode, const char*)
{
    g_payCallbackCalled = true;
    g_payCallbackCode = errorCode;
    return TRUE;
}

BOOL CALLBACK channelCallback(int errorCode, const GhomeChannelInfoAbiProbe* info)
{
    g_channelCallbackCalled = true;
    g_channelCallbackCode = errorCode;
    return errorCode == SDOL_ERRORCODE_OK && info && info->szChannelCode;
}

BOOL CALLBACK faceCallback(int errorCode, LPCSTR)
{
    g_faceCallbackCalled = true;
    g_faceCallbackCode = errorCode;
    return TRUE;
}

BOOL CALLBACK collectCallback(int errorCode, LPCSTR)
{
    g_collectCallbackCalled = true;
    g_collectCallbackCode = errorCode;
    return TRUE;
}

int CALLBACK loginCallback(int errorCode, const SDOLLoginResult* result, int userData, int)
{
    g_loginCallbackCalled = true;
    g_loginCallbackCode = errorCode;
    if (errorCode == SDOL_ERRORCODE_OK && result && result->SessionId) {
        g_loginCallbackSession = result->SessionId;
    }
    return userData;
}

bool verifyCryptoBridgeExports()
{
    HMODULE bridge = LoadLibraryW(L"qtlogin_sdobase_crypto.dll");
    if (!bridge) {
        std::wcerr << L"LoadLibrary(qtlogin_sdobase_crypto.dll) failed: " << GetLastError() << L"\n";
        return false;
    }

    const bool ok =
        GetProcAddress(bridge, "QtLogin_MakeLegacyPasswordTextBase64") != nullptr &&
        GetProcAddress(bridge, "QtLogin_SdoaEncryptWithDynamicKey") != nullptr;
    if (!ok) {
        std::wcerr << L"qtlogin_sdobase_crypto.dll is missing undecorated password crypto exports\n";
    }
    FreeLibrary(bridge);
    return ok;
}

bool verifyWebSdkCalls(LPSDOLGetModule getModule)
{
    SetEnvironmentVariableW(L"QTLOGIN_DISABLE_BROWSER_LAUNCH", L"1");

    ISDOLLogin7* login = nullptr;
    int code = getModule(__uuidof(ISDOLLogin7), reinterpret_cast<void**>(&login));
    if (code != SDOL_ERRORCODE_OK || !login) {
        std::wcerr << L"SDOLGetModule(ISDOLLogin7) failed, code=" << code << L"\n";
        return false;
    }

    code = login->OpenWindow(
        L"HTML",
        L"SDK module test browser",
        L"https://example.test/sdk-module-test",
        0,
        0,
        640,
        480,
        L"center");
    if (code != SDOL_ERRORCODE_OK) {
        std::wcerr << L"OpenWindow should be supported, code=" << code << L"\n";
        login->Release();
        return false;
    }

    g_payCallbackCalled = false;
    g_payCallbackCode = SDOL_ERRORCODE_FAILED;
    code = login->GhomePay(
        "{\"areaid\":\"1\",\"productid\":\"GWPAY-TEST\",\"gameorder\":\"ORDER-TEST\",\"extend\":\"test\",\"groupid\":\"1\"}",
        &payCallback);
    if (code != SDOL_ERRORCODE_OK || !g_payCallbackCalled || g_payCallbackCode != SDOL_ERRORCODE_OK) {
        std::wcerr << L"GhomePay should open and callback OK, code=" << code
                   << L" callback=" << g_payCallbackCalled
                   << L" callbackCode=" << g_payCallbackCode << L"\n";
        login->Release();
        return false;
    }

    login->Release();
    return true;
}

bool verifySdolLoginProcessRelaunch(LPSDOLGetModule getModule)
{
    ISDOLLogin7* login = nullptr;
    int code = getModule(__uuidof(ISDOLLogin7), reinterpret_cast<void**>(&login));
    if (code != SDOL_ERRORCODE_OK || !login) {
        std::wcerr << L"SDOLGetModule(ISDOLLogin7) for relaunch test failed, code=" << code << L"\n";
        return false;
    }

    SetEnvironmentVariableW(L"QTLOGIN_MOCK_SUCCESS", L"1");
    SetEnvironmentVariableW(L"QTLOGIN_TEST_EXIT_FIRST_LOGIN_PROCESS", L"1");
    g_loginCallbackCalled = false;
    g_loginCallbackCode = SDOL_ERRORCODE_FAILED;
    g_loginCallbackSession.clear();

    code = login->ShowLoginDialog(&loginCallback, SDOL_LOGINRESULT_CLOSE, 0);

    SetEnvironmentVariableW(L"QTLOGIN_TEST_EXIT_FIRST_LOGIN_PROCESS", nullptr);
    SetEnvironmentVariableW(L"QTLOGIN_MOCK_SUCCESS", nullptr);
    login->Release();

    if (code != SDOL_ERRORCODE_OK || !g_loginCallbackCalled || g_loginCallbackCode != SDOL_ERRORCODE_OK || g_loginCallbackSession.empty()) {
        std::wcerr << L"sdologin relaunch should recover first process exit, code=" << code
                   << L" callback=" << g_loginCallbackCalled
                   << L" callbackCode=" << g_loginCallbackCode
                   << L" session=" << g_loginCallbackSession << L"\n";
        return false;
    }
    return true;
}

bool verifySdoaAppCalls(LPigwGetModuleProbe getModule)
{
    SetEnvironmentVariableW(L"QTLOGIN_DISABLE_BROWSER_LAUNCH", L"1");

    ISDOAApp2AbiProbe* app = nullptr;
    if (!getModule(__uuidof(ISDOAApp2AbiProbe), reinterpret_cast<void**>(&app)) || !app) {
        std::wcerr << L"igwGetModule(ISDOAApp2) failed\n";
        return false;
    }

    int code = app->OpenWindow(
        L"HTML",
        L"SDOA app test browser",
        L"https://example.test/sdoa-app-test",
        0,
        0,
        640,
        480,
        L"center");
    if (code != 0) {
        std::wcerr << L"ISDOAApp::OpenWindow should be supported, code=" << code << L"\n";
        app->Release();
        return false;
    }

    code = app->ShowPaymentDialog(L"https://example.test/pay");
    if (code != 0) {
        std::wcerr << L"ISDOAApp::ShowPaymentDialog should be supported, code=" << code << L"\n";
        app->Release();
        return false;
    }

    BSTR signature = nullptr;
    code = app->GetClientSignature(L"demo-indication", &signature);
    if (code != 0 || !signature || SysStringLen(signature) == 0) {
        std::wcerr << L"ISDOAApp2::GetClientSignature should return a non-empty signature, code=" << code << L"\n";
        if (signature) {
            SysFreeString(signature);
        }
        app->Release();
        return false;
    }
    SysFreeString(signature);

    code = app->LoginDirect("session-direct", "additional-direct", 0);
    if (code != 0) {
        std::wcerr << L"ISDOAApp2::LoginDirect should be supported, code=" << code << L"\n";
        app->Release();
        return false;
    }

    if (app->GetLoginState(1) != 1) {
        std::wcerr << L"ISDOAApp::GetLoginState should report OK after LoginDirect\n";
        app->Release();
        return false;
    }

    app->Logout();
    if (app->GetLoginState(1) != 4) {
        std::wcerr << L"ISDOAApp::GetLoginState should report LOGOUT after Logout\n";
        app->Release();
        return false;
    }

    code = app->OpenWidget(L"demo.widget");
    if (code != 0 || app->WidgetExists(L"demo.widget") != 1) {
        std::wcerr << L"ISDOAApp widget calls should be stateful, code=" << code << L"\n";
        app->Release();
        return false;
    }
    code = app->CloseWidget(L"demo.widget");
    if (code != 0 || app->WidgetExists(L"demo.widget") != 0) {
        std::wcerr << L"ISDOAApp CloseWidget should clear widget state, code=" << code << L"\n";
        app->Release();
        return false;
    }

    app->Release();
    return true;
}

bool verifySdoaApp4Calls(LPigwGetModuleProbe getModule)
{
    SetEnvironmentVariableW(L"QTLOGIN_DISABLE_BROWSER_LAUNCH", L"1");

    ISDOAApp4AbiProbe* app = nullptr;
    if (!getModule(__uuidof(ISDOAApp4AbiProbe), reinterpret_cast<void**>(&app)) || !app) {
        std::wcerr << L"igwGetModule(ISDOAApp4) failed\n";
        return false;
    }

    int code = app->SetLoginMode(2);
    if (code != 0) {
        std::wcerr << L"ISDOAApp4::SetLoginMode should be supported, code=" << code << L"\n";
        app->Release();
        return false;
    }

    code = app->SetOwnerWindow(nullptr);
    if (code != 0) {
        std::wcerr << L"ISDOAApp4::SetOwnerWindow should be supported, code=" << code << L"\n";
        app->Release();
        return false;
    }

    code = app->MoveLoginDialog(24, 48);
    if (code != 0) {
        std::wcerr << L"ISDOAApp4::MoveLoginDialog should be supported, code=" << code << L"\n";
        app->Release();
        return false;
    }

    code = app->VertifyDoubleLogin(1);
    if (code != 0) {
        std::wcerr << L"ISDOAApp3::VertifyDoubleLogin should be supported, code=" << code << L"\n";
        app->Release();
        return false;
    }

    code = app->SetDpiSetting(144);
    if (code != 0) {
        std::wcerr << L"ISDOAApp4::SetDpiSetting should be supported, code=" << code << L"\n";
        app->Release();
        return false;
    }

    g_payCallbackCalled = false;
    g_payCallbackCode = SDOL_ERRORCODE_FAILED;
    code = app->GhomePay(
        "{\"areaid\":\"1\",\"productid\":\"GWPAY-791000855\",\"gameorder\":\"ORDER-DEMO\",\"extend\":\"test\",\"groupid\":\"1\"}",
        &payCallback);
    if (code != 0 || !g_payCallbackCalled || g_payCallbackCode != 0) {
        std::wcerr << L"ISDOAApp4::GhomePay should callback OK, code=" << code
                   << L" callback=" << g_payCallbackCalled
                   << L" callbackCode=" << g_payCallbackCode << L"\n";
        app->Release();
        return false;
    }

    g_channelCallbackCalled = false;
    g_channelCallbackCode = SDOL_ERRORCODE_FAILED;
    code = app->GhomeGetCPSChannelInfo(&channelCallback);
    if (code != 0 || !g_channelCallbackCalled || g_channelCallbackCode != 0) {
        std::wcerr << L"ISDOAApp4::GhomeGetCPSChannelInfo should callback OK, code=" << code
                   << L" callback=" << g_channelCallbackCalled
                   << L" callbackCode=" << g_channelCallbackCode << L"\n";
        app->Release();
        return false;
    }

    app->Release();
    return true;
}

bool verifySdoaApp5Calls(LPigwGetModuleProbe getModule)
{
    SetEnvironmentVariableW(L"QTLOGIN_DISABLE_BROWSER_LAUNCH", L"1");

    ISDOAApp5AbiProbe* app = nullptr;
    if (!getModule(__uuidof(ISDOAApp5AbiProbe), reinterpret_cast<void**>(&app)) || !app) {
        std::wcerr << L"igwGetModule(ISDOAApp5) failed\n";
        return false;
    }

    g_faceCallbackCalled = false;
    g_faceCallbackCode = SDOL_ERRORCODE_FAILED;
    int code = app->OpenFaceVerifyDialog(&faceCallback, L"csblacklist");
    if (code != 0 || !g_faceCallbackCalled || g_faceCallbackCode != 0) {
        std::wcerr << L"ISDOAApp5::OpenFaceVerifyDialog should callback OK, code=" << code
                   << L" callback=" << g_faceCallbackCalled
                   << L" callbackCode=" << g_faceCallbackCode << L"\n";
        app->Release();
        return false;
    }

    g_collectCallbackCalled = false;
    g_collectCallbackCode = SDOL_ERRORCODE_FAILED;
    code = app->OpenCollectUserMsgDialog(&collectCallback, L"1013");
    if (code != 0 || !g_collectCallbackCalled || g_collectCallbackCode != 0) {
        std::wcerr << L"ISDOAApp5::OpenCollectUserMsgDialog should callback OK, code=" << code
                   << L" callback=" << g_collectCallbackCalled
                   << L" callbackCode=" << g_collectCallbackCode << L"\n";
        app->Release();
        return false;
    }

    app->Release();
    return true;
}

}

int wmain()
{
    HMODULE sdk = LoadLibraryW(L"sdologinsdk.dll");
    if (!sdk) {
        std::wcerr << L"LoadLibrary(sdologinsdk.dll) failed: " << GetLastError() << L"\n";
        return 1;
    }

    auto initialize = reinterpret_cast<LPSDOLInitialize>(GetProcAddress(sdk, "SDOLInitialize"));
    auto getModule = reinterpret_cast<LPSDOLGetModule>(GetProcAddress(sdk, "SDOLGetModule"));
    auto terminal = reinterpret_cast<LPSDOLTerminal>(GetProcAddress(sdk, "SDOLTerminal"));
    auto igwInitialize = reinterpret_cast<LPigwInitializeProbe>(GetProcAddress(sdk, "igwInitialize"));
    auto igwGetModule = reinterpret_cast<LPigwGetModuleProbe>(GetProcAddress(sdk, "igwGetModule"));
    auto igwTerminal = reinterpret_cast<LPigwTerminalProbe>(GetProcAddress(sdk, "igwTerminal"));
    if (!initialize || !getModule || !terminal || !igwInitialize || !igwGetModule || !igwTerminal) {
        std::wcerr << L"SDK exports are missing\n";
        FreeLibrary(sdk);
        return 2;
    }

    SDOLAppInfo appInfo{};
    appInfo.Size = sizeof(appInfo);
    appInfo.AppID = 1;
    appInfo.AppName = L"QtLoginRewrite Module Test";
    appInfo.AppVer = L"1.0.0";
    appInfo.AreaId = -1;
    appInfo.GroupId = -1;

    const int initCode = initialize(&appInfo);
    if (initCode != SDOL_ERRORCODE_OK) {
        std::wcerr << L"SDOLInitialize failed, code=" << initCode << L"\n";
        FreeLibrary(sdk);
        return 3;
    }

    const bool ok =
        verifyCryptoBridgeExports() &&
        queryModule(getModule, __uuidof(ISDOLLogin7), L"ISDOLLogin7") &&
        queryModule(getModule, __uuidof(ISDOADx8AbiProbe), L"ISDOADx8") &&
        queryModule(getModule, __uuidof(ISDOADx9AbiProbe), L"ISDOADx9") &&
        queryModule(getModule, __uuidof(ISDOADx11AbiProbe), L"ISDOADx11") &&
        verifyWebSdkCalls(getModule) &&
        verifySdolLoginProcessRelaunch(getModule);

    terminal();

    AppInfoAbiProbe appInfo4{};
    appInfo4.cbSize = sizeof(appInfo4);
    appInfo4.nAppID = 1;
    appInfo4.pwcsAppName = L"QtLoginRewrite SDOA4 Test";
    appInfo4.pwcsAppVer = L"1.0.0";
    appInfo4.nRenderType = 0x20;
    appInfo4.nMaxUser = 1;
    appInfo4.nAreaId = -1;
    appInfo4.nGroupId = -1;

    const int igwInitCode = igwInitialize(0x0200, &appInfo4);
    if (igwInitCode != 0) {
        std::wcerr << L"igwInitialize(SDOA_SDK_VERSION, AppInfo) failed, code=" << igwInitCode << L"\n";
        FreeLibrary(sdk);
        return 5;
    }
    const bool igwOk =
        queryIgwModule(igwGetModule, __uuidof(ISDOADx9AbiProbe), L"ISDOADx9") &&
        verifySdoaAppCalls(igwGetModule) &&
        verifySdoaApp4Calls(igwGetModule) &&
        verifySdoaApp5Calls(igwGetModule);
    igwTerminal();
    FreeLibrary(sdk);

    if (!ok || !igwOk) {
        return 4;
    }

    std::wcout << L"sdk_module_tests passed\n";
    return 0;
}
