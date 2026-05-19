#pragma once

#include <cstddef>
#include <vector>

namespace qtlogin::ui_model {

enum class DialogKind {
    MainContainer,
    PasswordLogin,
    CodeKeyLogin,
    SmsLogin,
    PhoneRegisterLogin,
    PushMessageLogin,
    QuickAccountLogin,
    ThirdLogin,
    FeihuoLogin,
    GameUpdate,
    Protocol,
    Agreement,
    GenericBrowser,
    Payment,
    FaceVerify,
    FaceCollectUser,
    Geetest,
    MessageBox,
    MessageBoxOkCancel,
    MessageBoxOkCancelChecked,
    Captcha,
    ProtectCode,
    ProtectA8,
    ProtectD6,
    ProtectCard,
    ActiveCode,
    StaticLoginCondition,
    SecurityPhone,
    SecurityPhoneStaticLogin,
    GoDownSms,
    VoiceVerify,
    RealNameInfo,
    RealNameConfirm,
    GuardianInfo,
    GuardianConfirm,
    GuardianVoice,
    GuardianCaptcha,
    PlayerInfoCollect,
    LoginPrompt,
    LoginNotify,
    Keyboard,
    Ime,
    Tooltip,
    Cursor,
    Promotion,
    GameSetting,
    ThirdParty360,
    ThirdPartyMiGu,
    ThirdPartyShunWang,
    ThirdPartyXinYou,
    ThirdPartyFeiHuoWeb
};

struct LegacyDialogDescriptor {
    DialogKind kind;
    const wchar_t* stableName;
    const wchar_t* title;
    const wchar_t* oldClassName;
    const wchar_t* oldSkinFile;
    const wchar_t* category;
    const wchar_t* rewriteStatus;
};

const std::vector<LegacyDialogDescriptor>& legacyDialogs();
const LegacyDialogDescriptor* findLegacyDialog(DialogKind kind);
const LegacyDialogDescriptor* findLegacyDialogByName(const wchar_t* stableName);

}
