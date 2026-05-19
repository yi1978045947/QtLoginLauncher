#include "quick_login_skin.h"

namespace qtlogin::sdologin {

SkinButtonSpec sharedConnectButtonSpec()
{
    return SkinButtonSpec{
        281,
        58,
        104,
        52,
        "common/btn_login_n.png",
        "common/btn_login_c.png",
        "common/btn_login_c.png",
        "common/btn_login_c.png",
    };
}

QuickLoginSkinSpec quickLoginSkinSpec()
{
    QuickLoginSkinSpec spec{};
    spec.actionButton = sharedConnectButtonSpec();
    spec.retryButton = SkinButtonSpec{
        281,
        58,
        104,
        52,
        "push_message/6_a.png",
        "push_message/6_b.png",
        "push_message/6_c.png",
        "push_message/6_d.png",
    };
    spec.retryCountdownSeconds = 6;
    return spec;
}

MainLoginTabSpec mainLoginTabSpec()
{
    return MainLoginTabSpec{
        LoginPageSlot::Password,
        LoginPageSlot::PushMessage,
        LoginPageSlot::QrCode,
        LoginPageSlot::PushMessage,
    };
}

int loginPageIndex(LoginPageSlot slot)
{
    switch (slot) {
    case LoginPageSlot::Password:
        return 0;
    case LoginPageSlot::PushMessage:
        return 1;
    case LoginPageSlot::QrCode:
        return 2;
    case LoginPageSlot::SmsCode:
        return 3;
    }
    return 0;
}

}
