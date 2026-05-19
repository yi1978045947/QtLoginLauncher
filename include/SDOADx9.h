#ifndef QTLOGIN_REWRITE_SDOADX9_H
#define QTLOGIN_REWRITE_SDOADX9_H

#include <d3d9.h>

typedef interface ISDOADx9 ISDOADx9;
typedef ISDOADx9* PSDOADx9;
typedef ISDOADx9* LPSDOADx9;

MIDL_INTERFACE("59B789AB-BEE9-4C68-AA23-BCA9AB1A3E50")
ISDOADx9 : public IUnknown {
public:
    STDMETHOD_(HRESULT, Initialize)(THIS_ IDirect3DDevice9* pDev9, D3DPRESENT_PARAMETERS* pParams, bool bHookGameWnd) PURE;
    STDMETHOD_(HRESULT, Render)(THIS) PURE;
    STDMETHOD_(HRESULT, RenderEx)(THIS) PURE;
    STDMETHOD_(HRESULT, Finalize)(THIS) PURE;
    STDMETHOD_(void, OnDeviceReset)(THIS_ D3DPRESENT_PARAMETERS* pParams) PURE;
    STDMETHOD_(void, OnDeviceLost)(THIS) PURE;
    STDMETHOD_(HRESULT, OnWindowProc)(THIS_ HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* lResult) PURE;
};

#endif
