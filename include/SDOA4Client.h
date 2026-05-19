#ifndef QTLOGIN_REWRITE_SDOA4CLIENT_H
#define QTLOGIN_REWRITE_SDOA4CLIENT_H

#include <objbase.h>
#include <windows.h>

#pragma pack(push, 1)

#define SDOA_SDK_VERSION 0x0200

#define SDOA_OK 0
#define SDOA_FALSE 1
#define SDOA_INSUFFICIENT_BUFFER 122

#define SDOA_WM_CLIENT_RUN 0xA604
#define SDOA_WM_CLIENT_NOTIFY 0xA610
#define SDOA_NOTIFY_SHOWLOGIN 1
#define SDOA_NOTIFY_LOGINCLICK 2
#define SDOA_WM_HOST_NOTIFY 0xA611

#define SDOA_RENDERTYPE_UNKNOWN 0x00
#define SDOA_RENDERTYPE_GDI 0x01
#define SDOA_RENDERTYPE_DDRAW 0x04
#define SDOA_RENDERTYPE_D3D7 0x08
#define SDOA_RENDERTYPE_D3D8 0x10
#define SDOA_RENDERTYPE_D3D9 0x20
#define SDOA_RENDERTYPE_D3D10 0x40
#define SDOA_RENDERTYPE_OPENGL 0x80

#define SDOA_SCREEN_NONE 0
#define SDOA_SCREEN_NORMAL 1
#define SDOA_SCREEN_MINI 2

#define SDOA_WINDOWSSTATUS_NONE 0
#define SDOA_WINDOWSSTATUS_SHOW 1
#define SDOA_WINDOWSSTATUS_HIDE 2
#define SDOA_WINDOWSTYPE_NONE 0
#define SDOA_WINDOWSTYPE_TOOLBAR 1
#define SDOA_WINDOWSTYPE_WIDGET 2
#define SDOA_WINDOWSTYPE_WINDOWS 3

#define SDOA_OPENWIDGET_DEFAULT 0
#define SDOA_OPENWIDGET_NOBARICON 1

#define SDOA_CURSOR_AUTO 0
#define SDOA_CURSOR_SELF 1
#define SDOA_CURSOR_HIDESELF 2

#define SDOA_LOGINMETHOD_SSO 0
#define SDOA_LOGINMETHOD_SDO 1

#define SDOA_LOGINSTATE_UNKNOWN -1
#define SDOA_LOGINSTATE_NONE 0
#define SDOA_LOGINSTATE_OK 1
#define SDOA_LOGINSTATE_CONNECTING 2
#define SDOA_LOGINSTATE_FAILED 3
#define SDOA_LOGINSTATE_LOGOUT 4

#define SDOA_BARMODE_ICONBOTTOM 0
#define SDOA_BARMODE_ICONTOP 1

struct AppInfo {
    DWORD cbSize;
    int nAppID;
    LPCWSTR pwcsAppName;
    LPCWSTR pwcsAppVer;
    int nRenderType;
    int nMaxUser;
    int nAreaId;
    int nGroupId;
};

struct RoleInfo {
    DWORD cbSize;
    LPCWSTR pwcsRoleName;
    int nSex;
};

struct SDOAWinProperty {
    int nWinType;
    int nStatus;
    int nLeft;
    int nTop;
    int nWidth;
    int nHeight;
};

struct LoginResult {
    DWORD cbSize;
    LPCSTR szSessionId;
    LPCSTR szSndaid;
    LPCSTR szIdentityState;
    LPCSTR szAppendix;
    LPCSTR szAdditional;
};

struct LoginMessage {
    DWORD dwSize;
    int nErrorCode;
    BSTR* pbstrTitle;
    BSTR* pbstrContent;
};

#define SDOA_ERRORCODE_OK 0
#define SDOA_ERRORCODE_CANCEL -1
#define SDOA_ERRORCODE_UILOST -2
#define SDOA_ERRORCODE_FAILED -3
#define SDOA_ERRORCODE_UNKNOWN -4
#define SDOA_ERRORCODE_INVALIDPARAM -5
#define SDOA_ERRORCODE_UNEXCEPT -6
#define SDOA_ERRORCODE_ALREAYEXISTS -7
#define SDOA_ERRORCODE_SHOWMESSAGE -10

typedef BOOL(CALLBACK* LPLOGINCALLBACKPROC)(int nErrorCode, const LoginResult* pLoginResult, int nUserData, int nReserved);

typedef interface ISDOAApp ISDOAApp;
typedef ISDOAApp* PSDOAApp;
typedef ISDOAApp* LPSDOAApp;

MIDL_INTERFACE("C8187484-1C5F-48DF-8497-2136A72A5733")
ISDOAApp : public IUnknown {
public:
    STDMETHOD_(int, ShowLoginDialog)(THIS_ LPLOGINCALLBACKPROC fnCallback, int nUserData, int nReserved) PURE;
    STDMETHOD_(void, ModifyAppInfo)(THIS_ const AppInfo* pAppInfo) PURE;
    STDMETHOD_(void, Logout)(THIS) PURE;
    STDMETHOD_(int, GetLoginState)(THIS_ int nLoginMethod) PURE;
    STDMETHOD_(void, SetRoleInfo)(THIS_ const RoleInfo* pRoleInfo) PURE;
    STDMETHOD_(int, ShowPaymentDialog)(THIS_ LPCWSTR pwcsSrc) PURE;
    STDMETHOD_(int, GetScreenStatus)(THIS) PURE;
    STDMETHOD_(void, SetScreenStatus)(THIS_ int nValue) PURE;
    STDMETHOD_(bool, GetScreenEnabled)(THIS) PURE;
    STDMETHOD_(void, SetScreenEnabled)(THIS_ bool bValue) PURE;
    STDMETHOD_(bool, GetTaskBarPosition)(THIS_ PPOINT ptPosition) PURE;
    STDMETHOD_(bool, SetTaskBarPosition)(THIS_ const PPOINT ptPosition) PURE;
    STDMETHOD_(bool, GetFocus)(THIS) PURE;
    STDMETHOD_(void, SetFocus)(THIS_ bool bValue) PURE;
    STDMETHOD_(bool, HasUI)(THIS_ const PPOINT ptPosition) PURE;
    STDMETHOD_(int, GetTaskBarMode)(THIS) PURE;
    STDMETHOD_(void, SetTaskBarMode)(THIS_ int nBarMode) PURE;
    STDMETHOD_(int, GetSelfCursor)(THIS) PURE;
    STDMETHOD_(void, SetSelfCursor)(THIS_ int nValue) PURE;
    STDMETHOD_(int, OpenWidget)(THIS_ LPCWSTR pwcsWidgetNameSpace) PURE;
    STDMETHOD_(int, OpenWidgetEx)(THIS_ LPCWSTR pwcsWidgetNameSpace, LPCWSTR pwcsParam, int nFlag) PURE;
    STDMETHOD_(int, CloseWidget)(THIS_ LPCWSTR pwcsWidgetNameSpace) PURE;
    STDMETHOD_(int, ExecuteWidget)(THIS_ LPCWSTR pwcsWidgetNameSpaceOrGUID, LPCWSTR pwcsParam) PURE;
    STDMETHOD_(int, WidgetExists)(THIS_ LPCWSTR pwcsWidgetNameSpaceOrGUID) PURE;
    STDMETHOD_(int, OpenWindow)(THIS_ LPCWSTR pwcsWinType, LPCWSTR pwcsWinName, LPCWSTR pwcsSrc, int nLeft, int nTop, int nWidth, int nHeight, LPCWSTR pwcsMode) PURE;
    STDMETHOD_(int, CloseWindow)(THIS_ LPCWSTR pwcsWinName) PURE;
    STDMETHOD_(int, GetWinProperty)(THIS_ LPCWSTR pwcsWinName, SDOAWinProperty* pWinProperty) PURE;
    STDMETHOD_(int, SetWinProperty)(THIS_ LPCWSTR pwcsWinName, SDOAWinProperty* pWinProperty) PURE;
};

typedef interface ISDOAApp2 ISDOAApp2;
typedef ISDOAApp2* PSDOAApp2;
typedef ISDOAApp2* LPSDOAApp2;

MIDL_INTERFACE("3F35136C-7061-4E69-BF3A-CC78D026F48F")
ISDOAApp2 : public ISDOAApp {
public:
    STDMETHOD_(int, LoginDirect)(THIS_ LPCSTR szSessionId, LPCSTR szAdditional, int nReserved) PURE;
    STDMETHOD_(int, GetClientSignature)(THIS_ LPCWSTR szIndication, BSTR* Signature) PURE;
};

typedef DWORD(CALLBACK* LPGETAUDIOSOUNDVOLUME)();
typedef void(CALLBACK* LPSETAUDIOSOUNDVOLUME)(DWORD nNewVolume);
typedef DWORD(CALLBACK* LPGETAUDIOEFFECTVOLUME)();
typedef void(CALLBACK* LPSETAUDIOEFFECTVOLUME)(DWORD nNewVolume);

class ICommonChannel {
public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) = 0;
    virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
    virtual ULONG STDMETHODCALLTYPE Release() = 0;
    virtual void STDMETHODCALLTYPE SendData(BSTR Request, BSTR* Response) = 0;
    virtual void STDMETHODCALLTYPE Set_RevertChannel(ICommonChannel* pReverter) = 0;
    virtual ICommonChannel* STDMETHODCALLTYPE Get_RevertChannel() = 0;
    virtual void STDMETHODCALLTYPE Close() = 0;
    virtual void STDMETHODCALLTYPE Get_ChannelType(LPSTR szChannelType, DWORD nBufferLen) = 0;
};
typedef ICommonChannel* PCommonChannel;

typedef BOOL(WINAPI* LPCREATECHANNEL)(LPCSTR szChannelType, ICommonChannel** pChannel);

typedef interface ISDOAAppUtils ISDOAAppUtils;
typedef ISDOAAppUtils* PSDOAAppUtils;
typedef ISDOAAppUtils* LPSDOAAppUtils;

MIDL_INTERFACE("1170C2F9-28AD-4EA8-8392-E9A219C8FF65")
ISDOAAppUtils : public IUnknown {
public:
    STDMETHOD_(void, SetAudioSetting)(THIS_ LPGETAUDIOSOUNDVOLUME fnGetAudioSoundVolume,
        LPSETAUDIOSOUNDVOLUME fnSetAudioSoundVolume,
        LPGETAUDIOEFFECTVOLUME fnGetAudioEffectVolume,
        LPSETAUDIOEFFECTVOLUME fnSetAudioEffectVolume) PURE;
    STDMETHOD_(void, NodifyAudioChanged)(THIS) PURE;
    STDMETHOD_(void, SetCreateChannelCallback)(THIS_ const LPCREATECHANNEL fnCreateChannel) PURE;
};

typedef void(CALLBACK* LPQUERYCALLBACK)(int nRetCode, int nUserData);

typedef interface ISDOAClientService ISDOAClientService;
typedef ISDOAClientService* PSDOAClientService;
typedef ISDOAClientService* LPSDOAClientService;

MIDL_INTERFACE("AF56D291-823A-41AA-85A0-EBE5C6163425")
ISDOAClientService : public IUnknown {
public:
    STDMETHOD_(int, SetValue)(THIS_ const BSTR bstrKey, const BSTR bstrValue) PURE;
    STDMETHOD_(int, Query)(THIS_ const BSTR bstrService) PURE;
    STDMETHOD_(int, AsyncQuery)(THIS_ const BSTR bstrService, LPQUERYCALLBACK fnCallback, int nUserData) PURE;
    STDMETHOD_(int, GetValue)(THIS_ const BSTR bstrKey, BSTR* bstrValue) PURE;
};

typedef int(WINAPI* LPigwInitialize)(DWORD dwSdkVersion, const AppInfo* pAppInfo);
typedef bool(WINAPI* LPigwGetModule)(REFIID riid, void** intf);
typedef int(WINAPI* LPigwTerminal)();

#pragma pack(pop)

#endif
