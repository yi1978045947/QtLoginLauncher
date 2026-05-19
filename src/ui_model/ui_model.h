#pragma once

#include <cstdint>

namespace qtlogin::ui_model {

enum class LoginPage : uint32_t {
    Password = 1,
    QrCode = 2,
    Sms = 3,
    Push = 4,
    QuickAccount = 5,
    ThirdParty = 6
};

enum class ChallengeType : uint32_t {
    Captcha = 1,
    Geetest = 2,
    DynamicCode = 3,
    MatrixCard = 4,
    SecurityPhone = 5,
    RealName = 6,
    Guardian = 7,
    ActivationCode = 8
};

enum class WebDialogType : uint32_t {
    Protocol = 1,
    Generic = 2,
    Payment = 3,
    FaceVerify = 4,
    UserCollect = 5
};

}
