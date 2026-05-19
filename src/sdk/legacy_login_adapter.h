#pragma once

#include "legacy/interface.h"

namespace qtlogin::sdk {

class LegacyLoginAdapter final : public ISDOLLogin7 {
public:
    static LegacyLoginAdapter& instance();

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    STDMETHOD_(int, SetOwnerWindow)(HWND window) override;
    STDMETHOD_(int, ShowLoginDialog)(LPSDOLLOGINCALLBACKPROC callback, int userData, int reserved) override;
    STDMETHOD_(int, CloseLoginDialog)() override;
    STDMETHOD_(int, MoveLoginDialog)(int x, int y) override;
    STDMETHOD_(int, Logout)() override;

    STDMETHOD_(int, DoLogin)() override;
    STDMETHOD_(int, SetLoginMode)(int loginMode) override;
    STDMETHOD_(int, GetTicket)(BSTR* ticket, BSTR* sndaid) override;
    STDMETHOD_(int, ModifyAppInfo)(const SDOLAppInfo* appInfo) override;

    STDMETHOD_(int, RltLogin)(BSTR vkey) override;
    STDMETHOD_(int, LoginFeedback)(LPCWSTR sessionId, int result, int errorCode) override;
    STDMETHOD_(int, OpenWindow)(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode) override;
    STDMETHOD_(int, GetLoginedCompanyId)() override;

    STDMETHOD_(int, RegisterEvent)(LPLENOVOPAYWINDOWCLOSEDCALLBACKPROC callback) override;
    STDMETHOD_(int, ShowLenovoLoginDlg)(LPSDOLLOGINCALLBACKPROC callback, int userData, int reserved) override;
    STDMETHOD_(int, AsyncShowLenovoPayWindow)(LPCWSTR orderId, LPCWSTR productId, LPCWSTR groupId, LPCWSTR areaId, LPCWSTR roleId, LPCWSTR extend) override;

    STDMETHOD_(int, SetLoginDialogState)(int state) override;
    STDMETHOD_(int, SetGameClientType)(LPCWSTR gameClientType) override;
    STDMETHOD_(int, OpenActivityWindow)(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode) override;
    STDMETHOD_(int, AuthCodeLogin)(LPLOGINAUTHCODECALLBACKPROC callback, LPCSTR authCode) override;
    STDMETHOD_(int, OpenXinIeWindow)(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode) override;
    STDMETHOD_(int, OpenXinYouLoginIeAgain)() override;
    STDMETHOD_(int, XinYouLoginIeCenter)() override;
    STDMETHOD_(int, KeyBoardMessageNotify)(LPLOGINESCNOTIFYCALLBACKPROC callback) override;
    STDMETHOD_(int, SetDoubleLoginCallBack)(LPSDOLDOUBLELOGINCALLBACKPROC callback) override;
    STDMETHOD_(int, ModifyServerId)(LPCSTR serverId) override;
    STDMETHOD_(int, OpenFaceVertifyDlg)(LPFACEVERTIFYCALLBACKPROC callback, LPCWSTR vertifyType) override;
    STDMETHOD_(int, OpenCollectUserMsgDlg)(LPCOLLECTUSERMSGCALLBACKPROC callback, LPCWSTR collectMsgType) override;
    STDMETHOD_(int, GetTicketForAppid)(BSTR* ticket, BSTR* sndaid, int appId) override;
    STDMETHOD_(int, GhomePay)(const char* extend, LPLOGINPAYCALLBACKPROC callback) override;

private:
    LegacyLoginAdapter() = default;
};

}
