#ifndef QTLOGIN_REWRITE_SDOADX11_H
#define QTLOGIN_REWRITE_SDOADX11_H

#include <d3d11.h>

typedef interface ISDOADx11 ISDOADx11;
typedef ISDOADx11* PSDOADx11;
typedef ISDOADx11* LPSDOADx11;

MIDL_INTERFACE("7344D794-2D10-541C-F245-824F5A094B37")
ISDOADx11 : public IUnknown {
public:
    STDMETHOD_(HRESULT, Initialize)(THIS_ ID3D11Device* pDev11, IDXGISwapChain* pSwapChain, bool bHookGameWnd) PURE;
    STDMETHOD_(HRESULT, Render)(THIS) PURE;
    STDMETHOD_(HRESULT, RenderEx)(THIS) PURE;
    STDMETHOD_(HRESULT, Finalize)(THIS) PURE;
    STDMETHOD_(void, OnDeviceReset)(THIS_ IDXGISwapChain* pSwapChain) PURE;
    STDMETHOD_(void, OnDeviceLost)(THIS) PURE;
    STDMETHOD_(HRESULT, OnWindowProc)(THIS_ HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* lResult) PURE;
};

#endif
