#include "legacy_login_adapter.h"

#include <objbase.h>

#include "sdk_runtime.h"

namespace qtlogin::sdk {

LegacyLoginAdapter& LegacyLoginAdapter::instance()
{
    static LegacyLoginAdapter adapter;
    return adapter;
}

HRESULT LegacyLoginAdapter::QueryInterface(REFIID riid, void** object)
{
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;

    if (riid == IID_IUnknown) {
        *object = static_cast<IUnknown*>(static_cast<ISDOLLogin*>(this));
    } else if (riid == __uuidof(ISDOLLogin)) {
        *object = static_cast<ISDOLLogin*>(this);
    } else if (riid == __uuidof(ISDOLLogin2)) {
        *object = static_cast<ISDOLLogin2*>(this);
    } else if (riid == __uuidof(ISDOLLogin3)) {
        *object = static_cast<ISDOLLogin3*>(this);
    } else if (riid == __uuidof(ISDOLLogin4)) {
        *object = static_cast<ISDOLLogin4*>(this);
    } else if (riid == __uuidof(ISDOLLogin5)) {
        *object = static_cast<ISDOLLogin5*>(this);
    } else if (riid == __uuidof(ISDOLLogin6)) {
        *object = static_cast<ISDOLLogin6*>(this);
    } else if (riid == __uuidof(ISDOLLogin7)) {
        *object = static_cast<ISDOLLogin7*>(this);
    } else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG LegacyLoginAdapter::AddRef()
{
    return 1;
}

ULONG LegacyLoginAdapter::Release()
{
    return 1;
}

int LegacyLoginAdapter::SetOwnerWindow(HWND window)
{
    return SdkRuntime::instance().setOwnerWindow(window);
}

int LegacyLoginAdapter::ShowLoginDialog(LPSDOLLOGINCALLBACKPROC callback, int userData, int reserved)
{
    return SdkRuntime::instance().showLoginDialog(callback, userData, reserved);
}

int LegacyLoginAdapter::CloseLoginDialog()
{
    return SdkRuntime::instance().closeLoginDialog();
}

int LegacyLoginAdapter::MoveLoginDialog(int x, int y)
{
    return SdkRuntime::instance().moveLoginDialog(x, y);
}

int LegacyLoginAdapter::Logout()
{
    return SdkRuntime::instance().logout();
}

int LegacyLoginAdapter::DoLogin()
{
    return SDOL_ERRORCODE_NOT_SUPPORT;
}

int LegacyLoginAdapter::SetLoginMode(int loginMode)
{
    return SdkRuntime::instance().setLoginMode(loginMode);
}

int LegacyLoginAdapter::GetTicket(BSTR* ticket, BSTR* sndaid)
{
    return SdkRuntime::instance().getTicket(ticket, sndaid);
}

int LegacyLoginAdapter::ModifyAppInfo(const SDOLAppInfo* appInfo)
{
    return SdkRuntime::instance().modifyAppInfo(appInfo);
}

int LegacyLoginAdapter::RltLogin(BSTR)
{
    return SDOL_ERRORCODE_NOT_SUPPORT;
}

int LegacyLoginAdapter::LoginFeedback(LPCWSTR, int, int)
{
    return SDOL_ERRORCODE_OK;
}

int LegacyLoginAdapter::OpenWindow(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode)
{
    return SdkRuntime::instance().openWindow(winType, winName, src, left, top, width, height, mode);
}

int LegacyLoginAdapter::GetLoginedCompanyId()
{
    return 0;
}

int LegacyLoginAdapter::RegisterEvent(LPLENOVOPAYWINDOWCLOSEDCALLBACKPROC)
{
    return SDOL_ERRORCODE_OK;
}

int LegacyLoginAdapter::ShowLenovoLoginDlg(LPSDOLLOGINCALLBACKPROC callback, int userData, int reserved)
{
    return ShowLoginDialog(callback, userData, reserved);
}

int LegacyLoginAdapter::AsyncShowLenovoPayWindow(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR)
{
    return SDOL_ERRORCODE_NOT_SUPPORT;
}

int LegacyLoginAdapter::SetLoginDialogState(int)
{
    return SDOL_ERRORCODE_OK;
}

int LegacyLoginAdapter::SetGameClientType(LPCWSTR gameClientType)
{
    return SdkRuntime::instance().setGameClientType(gameClientType);
}

int LegacyLoginAdapter::OpenActivityWindow(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode)
{
    return SdkRuntime::instance().openWindow(winType, winName, src, left, top, width, height, mode);
}

int LegacyLoginAdapter::AuthCodeLogin(LPLOGINAUTHCODECALLBACKPROC, LPCSTR)
{
    return SDOL_ERRORCODE_NOT_SUPPORT;
}

int LegacyLoginAdapter::OpenXinIeWindow(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode)
{
    return SdkRuntime::instance().openWindow(winType, winName, src, left, top, width, height, mode);
}

int LegacyLoginAdapter::OpenXinYouLoginIeAgain()
{
    return SDOL_ERRORCODE_NOT_SUPPORT;
}

int LegacyLoginAdapter::XinYouLoginIeCenter()
{
    return SDOL_ERRORCODE_NOT_SUPPORT;
}

int LegacyLoginAdapter::KeyBoardMessageNotify(LPLOGINESCNOTIFYCALLBACKPROC)
{
    return SDOL_ERRORCODE_OK;
}

int LegacyLoginAdapter::SetDoubleLoginCallBack(LPSDOLDOUBLELOGINCALLBACKPROC)
{
    return SDOL_ERRORCODE_OK;
}

int LegacyLoginAdapter::ModifyServerId(LPCSTR serverId)
{
    return SdkRuntime::instance().modifyServerId(serverId);
}

int LegacyLoginAdapter::OpenFaceVertifyDlg(LPFACEVERTIFYCALLBACKPROC callback, LPCWSTR vertifyType)
{
    return SdkRuntime::instance().openFaceVerifyDialog(callback, vertifyType);
}

int LegacyLoginAdapter::OpenCollectUserMsgDlg(LPCOLLECTUSERMSGCALLBACKPROC callback, LPCWSTR collectMsgType)
{
    return SdkRuntime::instance().openCollectUserMsgDialog(callback, collectMsgType);
}

int LegacyLoginAdapter::GetTicketForAppid(BSTR* ticket, BSTR* sndaid, int)
{
    return GetTicket(ticket, sndaid);
}

int LegacyLoginAdapter::GhomePay(const char* extend, LPLOGINPAYCALLBACKPROC callback)
{
    return SdkRuntime::instance().ghomePay(extend, callback);
}

}
