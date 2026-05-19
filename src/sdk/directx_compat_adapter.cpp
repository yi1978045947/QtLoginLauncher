#include "directx_compat_adapter.h"

#include "logger.h"
#include "sdk_runtime.h"

#include "SDOADx8.h"
#include "SDOADx9.h"
#include "SDOADx11.h"

#include <mutex>

namespace qtlogin::sdk {
namespace {

struct OwnerWindowHook {
    HWND window = nullptr;
    WNDPROC previousProc = nullptr;
};

std::mutex g_hookMutex;
OwnerWindowHook g_ownerHook;

HWND hwndFromParams(D3DPRESENT_PARAMETERS* params)
{
    return params ? params->hDeviceWindow : nullptr;
}

POINT pointFromMouseLParam(LPARAM lParam)
{
    POINT point{};
    point.x = static_cast<short>(LOWORD(lParam));
    point.y = static_cast<short>(HIWORD(lParam));
    return point;
}

LPARAM makeMouseLParam(const POINT& point)
{
    return MAKELPARAM(static_cast<short>(point.x), static_cast<short>(point.y));
}

bool isForwardableMouseMessage(UINT message)
{
    switch (message) {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
        return true;
    default:
        return false;
    }
}

bool forwardMouseToEmbeddedChild(HWND owner, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (!isForwardableMouseMessage(message)) {
        return false;
    }

    POINT ownerPoint = pointFromMouseLParam(lParam);
    HWND child = ChildWindowFromPointEx(owner, ownerPoint, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
    if (!child || child == owner) {
        return false;
    }

    POINT childPoint = ownerPoint;
    ClientToScreen(owner, &childPoint);
    ScreenToClient(child, &childPoint);
    SetFocus(child);
    PostMessageW(child, message, wParam, makeMouseLParam(childPoint));
    if (result) {
        *result = 0;
    }
    return true;
}

LRESULT CALLBACK ownerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC previousProc = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_hookMutex);
        previousProc = (g_ownerHook.window == window) ? g_ownerHook.previousProc : nullptr;
    }

    LRESULT result = 0;
    if (forwardMouseToEmbeddedChild(window, message, wParam, lParam, &result)) {
        return result;
    }

    if (message == WM_NCDESTROY) {
        releaseDirectXCompatHooks();
    }

    return previousProc ? CallWindowProcW(previousProc, window, message, wParam, lParam)
                        : DefWindowProcW(window, message, wParam, lParam);
}

void installOwnerWindowHook(HWND window)
{
    if (!window) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_hookMutex);
    if (g_ownerHook.window == window) {
        return;
    }
    if (g_ownerHook.window && g_ownerHook.previousProc) {
        SetWindowLongPtrW(g_ownerHook.window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_ownerHook.previousProc));
        g_ownerHook = {};
    }

    WNDPROC previous = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&ownerWindowProc)));
    if (previous) {
        g_ownerHook.window = window;
        g_ownerHook.previousProc = previous;
        common::logLine(L"dx", L"owner window input hook installed");
    }
}

class DirectXCompatBase {
public:
    ULONG AddRefImpl() { return 1; }
    ULONG ReleaseImpl() { return 1; }

protected:
    HRESULT initializeOwner(HWND window, const wchar_t* component, bool hookGameWnd)
    {
        if (window) {
            SdkRuntime::instance().markDirectXOwnerWindow(window);
            common::logLine(component, L"DirectX owner window captured");
            if (hookGameWnd) {
                installOwnerWindowHook(window);
            }
        } else {
            common::logLine(component, L"DirectX initialize without owner window");
        }
        return S_OK;
    }

    HRESULT onWindowProcImpl(HWND, UINT, WPARAM, LPARAM, LRESULT* result)
    {
        if (result) {
            *result = 0;
        }
        return S_FALSE;
    }
};

class DirectX8Adapter final : public ISDOADx8, private DirectXCompatBase {
public:
    STDMETHOD(QueryInterface)(REFIID riid, void** object) override
    {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ISDOADx8)) {
            *object = static_cast<ISDOADx8*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return AddRefImpl(); }
    STDMETHOD_(ULONG, Release)() override { return ReleaseImpl(); }
    STDMETHOD_(HRESULT, Initialize)(IDirect3DDevice8*, D3DPRESENT_PARAMETERS* params, bool hookGameWnd) override { return initializeOwner(hwndFromParams(params), L"dx8", hookGameWnd); }
    STDMETHOD_(HRESULT, Render)() override { return S_OK; }
    STDMETHOD_(HRESULT, RenderEx)() override { return S_OK; }
    STDMETHOD_(HRESULT, Finalize)() override { releaseDirectXCompatHooks(); return S_OK; }
    STDMETHOD_(void, OnDeviceReset)(D3DPRESENT_PARAMETERS* params) override { SdkRuntime::instance().markDirectXOwnerWindow(hwndFromParams(params)); }
    STDMETHOD_(void, OnDeviceLost)() override {}
    STDMETHOD_(HRESULT, OnWindowProc)(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) override { return onWindowProcImpl(window, message, wParam, lParam, result); }
};

class DirectX9Adapter final : public ISDOADx9, private DirectXCompatBase {
public:
    STDMETHOD(QueryInterface)(REFIID riid, void** object) override
    {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ISDOADx9)) {
            *object = static_cast<ISDOADx9*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return AddRefImpl(); }
    STDMETHOD_(ULONG, Release)() override { return ReleaseImpl(); }
    STDMETHOD_(HRESULT, Initialize)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS* params, bool hookGameWnd) override { return initializeOwner(hwndFromParams(params), L"dx9", hookGameWnd); }
    STDMETHOD_(HRESULT, Render)() override { return S_OK; }
    STDMETHOD_(HRESULT, RenderEx)() override { return S_OK; }
    STDMETHOD_(HRESULT, Finalize)() override { releaseDirectXCompatHooks(); return S_OK; }
    STDMETHOD_(void, OnDeviceReset)(D3DPRESENT_PARAMETERS* params) override { SdkRuntime::instance().markDirectXOwnerWindow(hwndFromParams(params)); }
    STDMETHOD_(void, OnDeviceLost)() override {}
    STDMETHOD_(HRESULT, OnWindowProc)(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) override { return onWindowProcImpl(window, message, wParam, lParam, result); }
};

class DirectX11Adapter final : public ISDOADx11, private DirectXCompatBase {
public:
    STDMETHOD(QueryInterface)(REFIID riid, void** object) override
    {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ISDOADx11)) {
            *object = static_cast<ISDOADx11*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return AddRefImpl(); }
    STDMETHOD_(ULONG, Release)() override { return ReleaseImpl(); }
    STDMETHOD_(HRESULT, Initialize)(ID3D11Device*, IDXGISwapChain*, bool hookGameWnd) override { return initializeOwner(nullptr, L"dx11", hookGameWnd); }
    STDMETHOD_(HRESULT, Render)() override { return S_OK; }
    STDMETHOD_(HRESULT, RenderEx)() override { return S_OK; }
    STDMETHOD_(HRESULT, Finalize)() override { releaseDirectXCompatHooks(); return S_OK; }
    STDMETHOD_(void, OnDeviceReset)(IDXGISwapChain*) override {}
    STDMETHOD_(void, OnDeviceLost)() override {}
    STDMETHOD_(HRESULT, OnWindowProc)(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) override { return onWindowProcImpl(window, message, wParam, lParam, result); }
};

}

bool queryDirectXCompatModule(REFIID riid, void** intf)
{
    if (!intf) {
        return false;
    }
    *intf = nullptr;

    static DirectX8Adapter dx8;
    static DirectX9Adapter dx9;
    static DirectX11Adapter dx11;

    if (riid == __uuidof(ISDOADx8)) {
        *intf = static_cast<ISDOADx8*>(&dx8);
        return true;
    }
    if (riid == __uuidof(ISDOADx9)) {
        *intf = static_cast<ISDOADx9*>(&dx9);
        return true;
    }
    if (riid == __uuidof(ISDOADx11)) {
        *intf = static_cast<ISDOADx11*>(&dx11);
        return true;
    }
    return false;
}

void releaseDirectXCompatHooks()
{
    std::lock_guard<std::mutex> lock(g_hookMutex);
    if (g_ownerHook.window && g_ownerHook.previousProc) {
        SetWindowLongPtrW(g_ownerHook.window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_ownerHook.previousProc));
    }
    g_ownerHook = {};
}

}
