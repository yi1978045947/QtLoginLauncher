#include "dx_demo_behavior.h"

#include <algorithm>
#include <vector>

namespace qtlogin::samples {
namespace {

int scaledLegacyPixel(int value, double scale)
{
    if (scale <= 0.0) {
        scale = 1.0;
    }
    return static_cast<int>(value * scale + 0.5);
}

const wchar_t* safeText(const wchar_t* value)
{
    return value ? value : L"";
}

}

DxDemoBehavior defaultDxDemoBehavior()
{
    return DxDemoBehavior{1280, 720, true};
}

DxDemoRect centeredDxDemoWindowRect(int screenWidth, int screenHeight, const DxDemoBehavior& behavior)
{
    const int width = behavior.windowWidth;
    const int height = behavior.windowHeight;
    return DxDemoRect{
        std::max(0, (screenWidth - width) / 2),
        std::max(0, (screenHeight - height) / 2),
        width,
        height,
    };
}

DxDemoPoint dxDemoLoginDialogPosition(int clientWidth, int clientHeight, double legacyDpiScale)
{
    const int margin = 24;
    const int loginWidth = scaledLegacyPixel(442, legacyDpiScale);
    const int loginHeight = scaledLegacyPixel(404, legacyDpiScale);
    int panelRight = 0;
    for (const auto& button : dxDemoApiButtons()) {
        panelRight = std::max(panelRight, button.x + button.width);
    }
    panelRight += margin;

    const int rightAlignedX = clientWidth - loginWidth - margin;
    int preferredX = rightAlignedX;
    if (rightAlignedX < panelRight && panelRight + loginWidth <= clientWidth - margin) {
        preferredX = panelRight;
    }
    const int maxX = std::max(margin, clientWidth - loginWidth);
    const int x = std::max(margin, std::min(preferredX, maxX));

    const int centeredY = (clientHeight - loginHeight) / 2;
    const int maxY = std::max(32, clientHeight - loginHeight);
    const int y = std::max(32, std::min(centeredY, maxY));
    return DxDemoPoint{x, y};
}

std::wstring dxDemoLoginCallbackMessage(int errorCode, const wchar_t* userId, const wchar_t* sessionId)
{
    std::wstring message;
    if (errorCode == 0) {
        message = L"Login callback success";
    } else {
        message = L"Login callback error";
    }
    message += L"\r\nerrorCode=" + std::to_wstring(errorCode);
    message += L"\r\nuserid=";
    message += safeText(userId);
    message += L"\r\nsessionid=";
    message += safeText(sessionId);
    return message;
}

const std::vector<DxDemoApiButton>& dxDemoApiButtons()
{
    static const std::vector<DxDemoApiButton> buttons = {
        {DxDemoApiAction::ShowLoginDialog, L"ShowLoginDialog", 16, 14, 132, 28},
        {DxDemoApiAction::SetLoginMode, L"SetLoginMode", 156, 14, 116, 28},
        {DxDemoApiAction::SetOwnerWindow, L"SetOwnerWindow", 280, 14, 126, 28},
        {DxDemoApiAction::MoveLoginDialog, L"MoveLoginDialog", 414, 14, 132, 28},
        {DxDemoApiAction::Logout, L"Logout", 16, 50, 92, 28},
        {DxDemoApiAction::GetLoginState, L"GetLoginState", 116, 50, 118, 28},
        {DxDemoApiAction::ModifyAppInfo, L"ModifyAppInfo", 242, 50, 120, 28},
        {DxDemoApiAction::GetTicketForAppId, L"GetTicketForAppid", 370, 50, 146, 28},
        {DxDemoApiAction::SetDoubleLogin, L"SetDoubleLogin", 16, 86, 126, 28},
        {DxDemoApiAction::SetDpi, L"SetDpi(144)", 150, 86, 104, 28},
        {DxDemoApiAction::OpenPayQrWindow, L"OpenPayQR", 262, 86, 104, 28},
        {DxDemoApiAction::OpenWindow, L"OpenWindow", 374, 86, 112, 28},
        {DxDemoApiAction::ShowPaymentDialog, L"Payment", 16, 122, 92, 28},
        {DxDemoApiAction::OpenWidget, L"OpenWidget", 116, 122, 104, 28},
        {DxDemoApiAction::OpenFaceVerify, L"FaceVerify", 228, 122, 104, 28},
        {DxDemoApiAction::OpenFaceCollectMsg, L"FaceCollect", 340, 122, 110, 28},
        {DxDemoApiAction::GhomePay, L"GhomePay", 16, 158, 96, 28},
        {DxDemoApiAction::GhomeGetChannelCode, L"ChannelInfo", 120, 158, 106, 28},
        {DxDemoApiAction::CloseWidget, L"CloseWidget", 234, 158, 104, 28},
        {DxDemoApiAction::GetTicket, L"GetTicket", 346, 158, 92, 28},
        {DxDemoApiAction::IsdolOpenWindow, L"ISDOL OpenWindow", 16, 194, 132, 28},
    };
    return buttons;
}

}
