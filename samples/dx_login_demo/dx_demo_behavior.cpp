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
        {DxDemoApiAction::Logout, L"Logout", 156, 14, 92, 28},
        {DxDemoApiAction::GetLoginState, L"GetLoginState", 256, 14, 118, 28},
        {DxDemoApiAction::ModifyAppInfo, L"ModifyAppInfo", 382, 14, 120, 28},
        {DxDemoApiAction::OpenWindow, L"OpenWindow", 16, 50, 112, 28},
        {DxDemoApiAction::ShowPaymentDialog, L"Payment", 136, 50, 92, 28},
        {DxDemoApiAction::OpenWidget, L"OpenWidget", 236, 50, 104, 28},
        {DxDemoApiAction::CloseWidget, L"CloseWidget", 348, 50, 104, 28},
        {DxDemoApiAction::OpenFaceVerify, L"FaceVerify", 16, 86, 104, 28},
        {DxDemoApiAction::GhomePay, L"GhomePay", 128, 86, 96, 28},
        {DxDemoApiAction::GetTicket, L"GetTicket", 232, 86, 92, 28},
        {DxDemoApiAction::IsdolOpenWindow, L"ISDOL OpenWindow", 332, 86, 132, 28},
    };
    return buttons;
}

}
