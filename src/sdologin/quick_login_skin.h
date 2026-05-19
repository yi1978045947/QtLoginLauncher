#pragma once

#include <string>

namespace qtlogin::sdologin {

struct SkinButtonSpec {
    int x;
    int y;
    int width;
    int height;
    std::string normalImage;
    std::string hoverImage;
    std::string pressedImage;
    std::string disabledImage;
};

struct QuickLoginSkinSpec {
    SkinButtonSpec actionButton;
    SkinButtonSpec retryButton;
    int retryCountdownSeconds;
};

enum class LoginPageSlot {
    Password,
    PushMessage,
    QrCode,
    SmsCode,
};

struct MainLoginTabSpec {
    LoginPageSlot passwordTab;
    LoginPageSlot oneClickTab;
    LoginPageSlot qrTab;
    LoginPageSlot quickShortcut;
};

SkinButtonSpec sharedConnectButtonSpec();
QuickLoginSkinSpec quickLoginSkinSpec();
MainLoginTabSpec mainLoginTabSpec();
int loginPageIndex(LoginPageSlot slot);

}
