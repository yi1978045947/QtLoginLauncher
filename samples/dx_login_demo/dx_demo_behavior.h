#pragma once

#include <string>
#include <vector>

namespace qtlogin::samples {

struct DxDemoBehavior {
    int windowWidth;
    int windowHeight;
    bool autoShowLogin;
};

struct DxDemoRect {
    int x;
    int y;
    int width;
    int height;
};

struct DxDemoPoint {
    int x;
    int y;
};

enum class DxDemoApiAction {
    ShowLoginDialog = 2101,
    SetLoginMode,
    SetOwnerWindow,
    MoveLoginDialog,
    Logout,
    GetLoginState,
    ModifyAppInfo,
    GetTicketForAppId,
    SetDoubleLogin,
    SetDpi,
    OpenPayQrWindow,
    OpenWindow,
    ShowPaymentDialog,
    OpenWidget,
    CloseWidget,
    OpenFaceVerify,
    OpenFaceCollectMsg,
    GhomePay,
    GhomeGetChannelCode,
    GetTicket,
    IsdolOpenWindow,
};

struct DxDemoApiButton {
    DxDemoApiAction action;
    const wchar_t* text;
    int x;
    int y;
    int width;
    int height;
};

DxDemoBehavior defaultDxDemoBehavior();
DxDemoRect centeredDxDemoWindowRect(int screenWidth, int screenHeight, const DxDemoBehavior& behavior);
DxDemoPoint dxDemoLoginDialogPosition(int clientWidth, int clientHeight, double legacyDpiScale);
std::wstring dxDemoLoginCallbackMessage(int errorCode, const wchar_t* userId, const wchar_t* sessionId);
const std::vector<DxDemoApiButton>& dxDemoApiButtons();

}
