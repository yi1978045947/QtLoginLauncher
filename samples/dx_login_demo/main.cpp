#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include <windows.h>
#include <d3d9.h>
#include <oleauto.h>

#include "dx_demo_behavior.h"
#include "legacy/interface.h"
#include "SDOA4Client.h"
#include "SDOADx9.h"

namespace {

constexpr wchar_t kWindowClass[] = L"QtLoginRewriteDx9DemoWindow";
constexpr wchar_t kManualUrl[] = L"https://www.daoyu8.com/#/";

IDirect3D9* g_d3d = nullptr;
IDirect3DDevice9* g_device = nullptr;
D3DPRESENT_PARAMETERS g_present = {};
std::atomic_bool g_loginRunning = false;
HWND g_mainWindow = nullptr;

std::mutex g_sdkMutex;
HMODULE g_sdk = nullptr;
LPigwInitialize g_igwInitialize = nullptr;
LPigwGetModule g_igwGetModule = nullptr;
LPigwTerminal g_igwTerminal = nullptr;
LPSDOLGetModule g_sdolGetModule = nullptr;
ISDOADx9* g_dx9 = nullptr;
ISDOAApp2* g_sdoaApp = nullptr;
ISDOLLogin7* g_loginExt = nullptr;
bool g_sdkReady = false;

using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(HANDLE);
using GetDpiForWindowFn = UINT(WINAPI*)(HWND);

#ifndef DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE reinterpret_cast<HANDLE>(-2)
#endif

void configureDpiAwareness()
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    auto* fn = user32 ? reinterpret_cast<SetProcessDpiAwarenessContextFn>(
                            GetProcAddress(user32, "SetProcessDpiAwarenessContext"))
                      : nullptr;
    if (!fn || !fn(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)) {
        SetProcessDPIAware();
    }
}

std::wstring utf8ToWide(const char* value)
{
    if (!value || !value[0]) {
        return std::wstring();
    }
    const int required = MultiByteToWideChar(CP_UTF8, 0, value, -1, nullptr, 0);
    if (required <= 0) {
        return std::wstring(value, value + strlen(value));
    }
    std::wstring result(static_cast<size_t>(required - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value, -1, result.data(), required);
    return result;
}

void setStatus(HWND hwnd, const std::wstring& text)
{
    SetWindowTextW(hwnd, (L"DX9 SDK Demo - " + text + L" - Press L to login").c_str());
}

void showApiResult(HWND hwnd, const wchar_t* apiName, int code, const std::wstring& detail = std::wstring())
{
    std::wstring message = apiName;
    message += L"\r\ncode=" + std::to_wstring(code);
    if (!detail.empty()) {
        message += L"\r\n";
        message += detail;
    }
    MessageBoxW(hwnd, message.c_str(), L"DX demo SDK call", MB_OK | (code == 0 ? MB_ICONINFORMATION : MB_ICONWARNING));
}

double legacyDpiScaleForDemoWindow(HWND hwnd)
{
    UINT dpi = 0;
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    auto* getDpiForWindow = user32 ? reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(user32, "GetDpiForWindow")) : nullptr;
    if (getDpiForWindow) {
        dpi = getDpiForWindow(hwnd);
    }
    if (dpi == 0) {
        HDC screen = GetDC(hwnd);
        if (screen) {
            dpi = static_cast<UINT>(GetDeviceCaps(screen, LOGPIXELSX));
            ReleaseDC(hwnd, screen);
        }
    }
    if (dpi == 0) {
        dpi = 96;
    }
    return static_cast<double>(dpi) / 96.0;
}

void positionLoginDialogForDemo(HWND hwnd)
{
    if (!g_loginExt) {
        return;
    }
    RECT client{};
    GetClientRect(hwnd, &client);
    const qtlogin::samples::DxDemoPoint position = qtlogin::samples::dxDemoLoginDialogPosition(
        client.right - client.left,
        client.bottom - client.top,
        legacyDpiScaleForDemoWindow(hwnd));
    g_loginExt->MoveLoginDialog(position.x, position.y);
}

BOOL CALLBACK sdoaLoginCallback(int errorCode, const LoginResult* result, int, int)
{
    const std::wstring userId = result ? utf8ToWide(result->szSndaid) : std::wstring();
    const std::wstring sessionId = result ? utf8ToWide(result->szSessionId) : std::wstring();
    const std::wstring message = qtlogin::samples::dxDemoLoginCallbackMessage(errorCode, userId.c_str(), sessionId.c_str());
    OutputDebugStringW((message + L"\n").c_str());
    if (g_mainWindow) {
        if (errorCode == SDOA_ERRORCODE_OK && result) {
            setStatus(g_mainWindow, L"SDOA login success sndaid=" + userId + L" session=" + sessionId);
            MessageBoxW(g_mainWindow, message.c_str(), L"ISDOAApp::ShowLoginDialog callback", MB_OK | MB_ICONINFORMATION);
        } else {
            setStatus(g_mainWindow, L"SDOA login callback error=" + std::to_wstring(errorCode));
            MessageBoxW(g_mainWindow, message.c_str(), L"ISDOAApp::ShowLoginDialog callback", MB_OK | MB_ICONWARNING);
        }
    }
    return TRUE;
}

int CALLBACK sdolLoginCallback(int errorCode, const SDOLLoginResult* result, int, int)
{
    const wchar_t* userId = result ? result->Sndaid : nullptr;
    const wchar_t* sessionId = result ? result->SessionId : nullptr;
    const std::wstring message = qtlogin::samples::dxDemoLoginCallbackMessage(errorCode, userId, sessionId);
    if (g_mainWindow) {
        MessageBoxW(g_mainWindow, message.c_str(), L"ISDOLLogin7 callback", MB_OK | (errorCode == 0 ? MB_ICONINFORMATION : MB_ICONWARNING));
    }
    return SDOL_LOGINRESULT_CLOSE;
}

BOOL CALLBACK faceCallback(int errorCode, LPCSTR token)
{
    if (g_mainWindow) {
        showApiResult(g_mainWindow, L"ISDOLLogin7::OpenFaceVertifyDlg callback", errorCode, L"token=" + utf8ToWide(token));
    }
    return TRUE;
}

BOOL CALLBACK payCallback(int errorCode, const char* message)
{
    if (g_mainWindow) {
        showApiResult(g_mainWindow, L"ISDOLLogin7::GhomePay callback", errorCode, L"message=" + utf8ToWide(message));
    }
    return TRUE;
}

bool createDevice(HWND hwnd)
{
    g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_d3d) {
        return false;
    }

    ZeroMemory(&g_present, sizeof(g_present));
    g_present.Windowed = TRUE;
    g_present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_present.BackBufferFormat = D3DFMT_UNKNOWN;
    g_present.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    g_present.hDeviceWindow = hwnd;

    HRESULT hr = g_d3d->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        hwnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &g_present,
        &g_device);

    if (FAILED(hr)) {
        hr = g_d3d->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_REF,
            hwnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &g_present,
            &g_device);
    }

    return SUCCEEDED(hr);
}

void releaseDevice()
{
    if (g_device) {
        g_device->Release();
        g_device = nullptr;
    }
    if (g_d3d) {
        g_d3d->Release();
        g_d3d = nullptr;
    }
}

void releaseSdkSession()
{
    std::lock_guard<std::mutex> lock(g_sdkMutex);
    if (g_dx9) {
        g_dx9->Finalize();
        g_dx9->Release();
        g_dx9 = nullptr;
    }
    if (g_sdoaApp) {
        g_sdoaApp->Release();
        g_sdoaApp = nullptr;
    }
    if (g_loginExt) {
        g_loginExt->Release();
        g_loginExt = nullptr;
    }
    if (g_igwTerminal) {
        g_igwTerminal();
    }
    if (g_sdk) {
        FreeLibrary(g_sdk);
    }
    g_sdk = nullptr;
    g_igwInitialize = nullptr;
    g_igwGetModule = nullptr;
    g_igwTerminal = nullptr;
    g_sdolGetModule = nullptr;
    g_sdkReady = false;
}

bool ensureSdkSession(HWND hwnd)
{
    std::lock_guard<std::mutex> lock(g_sdkMutex);
    if (g_sdkReady) {
        return true;
    }

    g_sdk = LoadLibraryW(L"sdologinsdk.dll");
    if (!g_sdk) {
        setStatus(hwnd, L"LoadLibrary(sdologinsdk.dll) failed");
        return false;
    }

    g_igwInitialize = reinterpret_cast<LPigwInitialize>(GetProcAddress(g_sdk, "igwInitialize"));
    g_igwGetModule = reinterpret_cast<LPigwGetModule>(GetProcAddress(g_sdk, "igwGetModule"));
    g_igwTerminal = reinterpret_cast<LPigwTerminal>(GetProcAddress(g_sdk, "igwTerminal"));
    g_sdolGetModule = reinterpret_cast<LPSDOLGetModule>(GetProcAddress(g_sdk, "SDOLGetModule"));
    if (!g_igwInitialize || !g_igwGetModule || !g_igwTerminal || !g_sdolGetModule) {
        setStatus(hwnd, L"sdk exports missing");
        FreeLibrary(g_sdk);
        g_sdk = nullptr;
        return false;
    }

    AppInfo appInfo{};
    appInfo.cbSize = sizeof(appInfo);
    appInfo.nAppID = 1;
    appInfo.pwcsAppName = L"DX9 Login Demo";
    appInfo.pwcsAppVer = L"1.0.0";
    appInfo.nRenderType = SDOA_RENDERTYPE_D3D9;
    appInfo.nMaxUser = 1;
    appInfo.nAreaId = -1;
    appInfo.nGroupId = -1;

    const int initCode = g_igwInitialize(SDOA_SDK_VERSION, &appInfo);
    if (initCode != SDOA_OK) {
        setStatus(hwnd, L"igwInitialize failed code=" + std::to_wstring(initCode));
        FreeLibrary(g_sdk);
        g_sdk = nullptr;
        return false;
    }

    if (!g_igwGetModule(__uuidof(ISDOADx9), reinterpret_cast<void**>(&g_dx9)) || !g_dx9) {
        setStatus(hwnd, L"igwGetModule(ISDOADx9) failed");
        g_igwTerminal();
        FreeLibrary(g_sdk);
        g_sdk = nullptr;
        return false;
    }
    g_dx9->Initialize(g_device, &g_present, true);

    if (!g_igwGetModule(__uuidof(ISDOAApp2), reinterpret_cast<void**>(&g_sdoaApp)) || !g_sdoaApp) {
        setStatus(hwnd, L"igwGetModule(ISDOAApp2) failed");
        g_dx9->Release();
        g_dx9 = nullptr;
        g_igwTerminal();
        FreeLibrary(g_sdk);
        g_sdk = nullptr;
        return false;
    }

    if (g_sdolGetModule(__uuidof(ISDOLLogin7), reinterpret_cast<void**>(&g_loginExt)) == SDOL_ERRORCODE_OK && g_loginExt) {
        g_loginExt->SetOwnerWindow(hwnd);
        g_loginExt->SetLoginMode(AttachToLoginMode);
    }

    g_sdkReady = true;
    setStatus(hwnd, L"igw session ready");
    return true;
}

void render()
{
    if (!g_device) {
        return;
    }

    const DWORD ticks = GetTickCount();
    const int blue = 64 + static_cast<int>((ticks / 12) % 96);
    const D3DCOLOR color = D3DCOLOR_XRGB(25, 38, blue);
    g_device->Clear(0, nullptr, D3DCLEAR_TARGET, color, 1.0f, 0);
    if (SUCCEEDED(g_device->BeginScene())) {
        if (g_dx9) {
            g_dx9->RenderEx();
        }
        g_device->EndScene();
    }
    if (g_dx9) {
        g_dx9->Render();
    }
    g_device->Present(nullptr, nullptr, nullptr, nullptr);
}

void runSdoaLogin(HWND hwnd)
{
    if (g_loginRunning.exchange(true)) {
        return;
    }

    if (!ensureSdkSession(hwnd)) {
        g_loginRunning = false;
        return;
    }

    positionLoginDialogForDemo(hwnd);
    setStatus(hwnd, L"ISDOAApp::ShowLoginDialog");
    const int code = g_sdoaApp->ShowLoginDialog(&sdoaLoginCallback, 9001, 0);
    if (code == SDOA_ERRORCODE_OK) {
        setStatus(hwnd, L"ISDOAApp::ShowLoginDialog returned OK");
    } else {
        setStatus(hwnd, L"ISDOAApp::ShowLoginDialog returned " + std::to_wstring(code));
    }
    g_loginRunning = false;
}

void createApiButtons(HWND hwnd)
{
    CreateWindowExW(
        0,
        L"STATIC",
        L"SDOA4Client / ISDOL API",
        WS_CHILD | WS_VISIBLE,
        16,
        120,
        240,
        20,
        hwnd,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);

    for (const auto& button : qtlogin::samples::dxDemoApiButtons()) {
        CreateWindowExW(
            0,
            L"BUTTON",
            button.text,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            button.x,
            button.y,
            button.width,
            button.height,
            hwnd,
            reinterpret_cast<HMENU>(static_cast<int>(button.action)),
            GetModuleHandleW(nullptr),
            nullptr);
    }
}

void handleApiAction(HWND hwnd, qtlogin::samples::DxDemoApiAction action)
{
    if (action == qtlogin::samples::DxDemoApiAction::ShowLoginDialog) {
        std::thread([hwnd]() { runSdoaLogin(hwnd); }).detach();
        return;
    }

    if (!ensureSdkSession(hwnd)) {
        return;
    }

    switch (action) {
    case qtlogin::samples::DxDemoApiAction::Logout:
        g_sdoaApp->Logout();
        showApiResult(hwnd, L"ISDOAApp::Logout", 0);
        break;
    case qtlogin::samples::DxDemoApiAction::GetLoginState: {
        const int state = g_sdoaApp->GetLoginState(SDOA_LOGINMETHOD_SDO);
        showApiResult(hwnd, L"ISDOAApp::GetLoginState", 0, L"state=" + std::to_wstring(state));
        break;
    }
    case qtlogin::samples::DxDemoApiAction::ModifyAppInfo: {
        AppInfo appInfo{};
        appInfo.cbSize = sizeof(appInfo);
        appInfo.nAppID = 1;
        appInfo.pwcsAppName = L"DX9 Login Demo Modified";
        appInfo.pwcsAppVer = L"1.0.1";
        appInfo.nRenderType = SDOA_RENDERTYPE_D3D9;
        appInfo.nMaxUser = 1;
        appInfo.nAreaId = -1;
        appInfo.nGroupId = -1;
        g_sdoaApp->ModifyAppInfo(&appInfo);
        showApiResult(hwnd, L"ISDOAApp::ModifyAppInfo", 0, L"AppVer=1.0.1");
        break;
    }
    case qtlogin::samples::DxDemoApiAction::OpenWindow: {
        const int code = g_sdoaApp->OpenWindow(L"HTML", L"SDOA OpenWindow", kManualUrl, 80, 80, 900, 640, L"center");
        showApiResult(hwnd, L"ISDOAApp::OpenWindow", code, kManualUrl);
        break;
    }
    case qtlogin::samples::DxDemoApiAction::ShowPaymentDialog: {
        const int code = g_sdoaApp->ShowPaymentDialog(kManualUrl);
        showApiResult(hwnd, L"ISDOAApp::ShowPaymentDialog", code, kManualUrl);
        break;
    }
    case qtlogin::samples::DxDemoApiAction::OpenWidget: {
        const int code = g_sdoaApp->OpenWidget(L"demo.widget");
        showApiResult(hwnd, L"ISDOAApp::OpenWidget", code, L"WidgetExists=" + std::to_wstring(g_sdoaApp->WidgetExists(L"demo.widget")));
        break;
    }
    case qtlogin::samples::DxDemoApiAction::CloseWidget: {
        const int code = g_sdoaApp->CloseWidget(L"demo.widget");
        showApiResult(hwnd, L"ISDOAApp::CloseWidget", code, L"WidgetExists=" + std::to_wstring(g_sdoaApp->WidgetExists(L"demo.widget")));
        break;
    }
    case qtlogin::samples::DxDemoApiAction::OpenFaceVerify:
        if (g_loginExt) {
            const int code = g_loginExt->OpenFaceVertifyDlg(&faceCallback, kManualUrl);
            showApiResult(hwnd, L"ISDOLLogin7::OpenFaceVertifyDlg", code, kManualUrl);
        } else {
            showApiResult(hwnd, L"ISDOLLogin7::OpenFaceVertifyDlg", SDOL_ERRORCODE_NOT_SUPPORT);
        }
        break;
    case qtlogin::samples::DxDemoApiAction::GhomePay:
        if (g_loginExt) {
            const int code = g_loginExt->GhomePay("{\"url\":\"https://www.daoyu8.com/#/\",\"productid\":\"GWPAY-TEST\",\"areaid\":\"1\",\"groupid\":\"1\",\"gameorder\":\"ORDER-DEMO\"}", &payCallback);
            showApiResult(hwnd, L"ISDOLLogin7::GhomePay", code);
        } else {
            showApiResult(hwnd, L"ISDOLLogin7::GhomePay", SDOL_ERRORCODE_NOT_SUPPORT);
        }
        break;
    case qtlogin::samples::DxDemoApiAction::GetTicket:
        if (g_loginExt) {
            BSTR ticket = nullptr;
            BSTR sndaid = nullptr;
            const int code = g_loginExt->GetTicket(&ticket, &sndaid);
            std::wstring detail;
            if (ticket) {
                detail += L"ticket=";
                detail += ticket;
            }
            if (sndaid) {
                detail += L"\r\nsndaid=";
                detail += sndaid;
            }
            showApiResult(hwnd, L"ISDOLLogin7::GetTicket", code, detail);
            if (ticket) {
                SysFreeString(ticket);
            }
            if (sndaid) {
                SysFreeString(sndaid);
            }
        } else {
            showApiResult(hwnd, L"ISDOLLogin7::GetTicket", SDOL_ERRORCODE_NOT_SUPPORT);
        }
        break;
    case qtlogin::samples::DxDemoApiAction::IsdolOpenWindow:
        if (g_loginExt) {
            const int code = g_loginExt->OpenWindow(L"HTML", L"ISDOL OpenWindow", kManualUrl, 90, 90, 900, 640, L"center");
            showApiResult(hwnd, L"ISDOLLogin7::OpenWindow", code, kManualUrl);
        } else {
            showApiResult(hwnd, L"ISDOLLogin7::OpenWindow", SDOL_ERRORCODE_NOT_SUPPORT);
        }
        break;
    default:
        break;
    }
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (g_dx9) {
        LRESULT dxResult = 0;
        if (g_dx9->OnWindowProc(hwnd, message, wParam, lParam, &dxResult) == S_OK) {
            return dxResult;
        }
    }

    switch (message) {
    case WM_CREATE:
        setStatus(hwnd, L"ready");
        createApiButtons(hwnd);
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    case WM_TIMER:
        render();
        return 0;
    case WM_COMMAND:
        handleApiAction(hwnd, static_cast<qtlogin::samples::DxDemoApiAction>(LOWORD(wParam)));
        return 0;
    case WM_KEYDOWN:
        if (wParam == 'L') {
            std::thread([hwnd]() { runSdoaLogin(hwnd); }).detach();
            return 0;
        }
        break;
    case WM_LBUTTONDBLCLK:
        std::thread([hwnd]() { runSdoaLogin(hwnd); }).detach();
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        releaseSdkSession();
        releaseDevice();
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    configureDpiAwareness();

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kWindowClass;
    RegisterClassExW(&wc);

    const qtlogin::samples::DxDemoBehavior behavior = qtlogin::samples::defaultDxDemoBehavior();
    const qtlogin::samples::DxDemoRect windowRect = qtlogin::samples::centeredDxDemoWindowRect(
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        behavior);

    HWND hwnd = CreateWindowExW(
        0,
        kWindowClass,
        L"DX9 SDK Demo - Press L to login",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        windowRect.x,
        windowRect.y,
        windowRect.width,
        windowRect.height,
        nullptr,
        nullptr,
        instance,
        nullptr);
    if (!hwnd) {
        return 1;
    }
    g_mainWindow = hwnd;

    if (!createDevice(hwnd)) {
        MessageBoxW(hwnd, L"Direct3D9 device creation failed.", L"dx_login_demo", MB_ICONERROR);
        DestroyWindow(hwnd);
        return 2;
    }

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);
    if (behavior.autoShowLogin) {
        std::thread([hwnd]() { runSdoaLogin(hwnd); }).detach();
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    g_mainWindow = nullptr;
    return static_cast<int>(msg.wParam);
}
