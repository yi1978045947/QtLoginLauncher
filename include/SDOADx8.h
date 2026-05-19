#ifndef QTLOGIN_REWRITE_SDOADX8_H
#define QTLOGIN_REWRITE_SDOADX8_H

#include <d3d9types.h>
#include <objbase.h>
#include <windows.h>

struct IDirect3DDevice8;

typedef interface ISDOADx8 ISDOADx8;
typedef ISDOADx8* PSDOADx8;
typedef ISDOADx8* LPSDOADx8;

MIDL_INTERFACE("6469AE4A-4862-4248-913C-704179AFE43E")
ISDOADx8 : public IUnknown {
public:
    STDMETHOD_(HRESULT, Initialize)(THIS_ IDirect3DDevice8* pDev8, D3DPRESENT_PARAMETERS* pParams, bool bHookGameWnd) PURE;
    STDMETHOD_(HRESULT, Render)(THIS) PURE;
    STDMETHOD_(HRESULT, RenderEx)(THIS) PURE;
    STDMETHOD_(HRESULT, Finalize)(THIS) PURE;
    STDMETHOD_(void, OnDeviceReset)(THIS_ D3DPRESENT_PARAMETERS* pParams) PURE;
    STDMETHOD_(void, OnDeviceLost)(THIS) PURE;
    STDMETHOD_(HRESULT, OnWindowProc)(THIS_ HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* lResult) PURE;
};

#endif
