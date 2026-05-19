#include "legacy_dialog_registry.h"

#include <cwchar>

namespace qtlogin::ui_model {
namespace {

const std::vector<LegacyDialogDescriptor> kDialogs = {
    {DialogKind::MainContainer, L"main_container", L"登录主窗口", L"ContainerDlg", L"container/UIDialog.xml", L"login", L"Qt 主窗口已建，继续补齐旧按钮行为"},
    {DialogKind::PasswordLogin, L"password_login", L"账号密码登录", L"CLoginDlg", L"login/UIDialog.xml", L"login", L"已建基础页，待接真实认证"},
    {DialogKind::CodeKeyLogin, L"code_key_login", L"扫码登录", L"CCodeKeyLoginDlg", L"code_key/UIDialog.xml", L"login", L"已建基础页，待接二维码服务"},
    {DialogKind::SmsLogin, L"sms_login", L"短信登录", L"CSMSLoginDlg", L"sms_login/UIDialog.xml", L"login", L"已建基础页，待接短信服务"},
    {DialogKind::PhoneRegisterLogin, L"phone_register_login", L"手机验证码登录", L"CPhoneRegisterDlg", L"cellphone_register/UIDialog.xml", L"login", L"骨架登记"},
    {DialogKind::PushMessageLogin, L"push_message_login", L"一键登录/叨鱼确认", L"CPushMessageLoginDlg", L"push_message/UIDialog.xml", L"login", L"Qt 页迁移中"},
    {DialogKind::QuickAccountLogin, L"quick_account_login", L"保存账号快速登录", L"CSecureUnisignonDlg", L"accountlst/UIDialog.xml", L"login", L"骨架登记"},
    {DialogKind::ThirdLogin, L"third_login", L"第三方登录入口", L"ThirdLoginDlg", L"thrid_login/UIDialog.xml", L"third_party", L"骨架登记"},
    {DialogKind::FeihuoLogin, L"feihuo_login", L"飞火登录", L"CFeihuoDlg", L"feihuo/UIDialog.xml", L"third_party", L"骨架登记"},
    {DialogKind::GameUpdate, L"game_update", L"游戏更新", L"CGameUpdateDlg", L"gameupdate/UIDialog.xml", L"update", L"更新工程骨架已建，UI 骨架登记"},
    {DialogKind::Protocol, L"protocol", L"协议浏览", L"CProtocolDlg", L"protocol/UIDialog.xml", L"browser", L"已建协议弹窗占位，待接 WebView"},
    {DialogKind::Agreement, L"agreement", L"用户协议浏览", L"CAgreementDlg", L"agreementie/UIDialog.xml", L"browser", L"骨架登记"},
    {DialogKind::GenericBrowser, L"generic_browser", L"通用网页窗口", L"CIeDlg", L"ie/UIDialog.xml", L"browser", L"骨架登记"},
    {DialogKind::Payment, L"payment", L"支付窗口", L"CPayMentDlg", L"CPayMentDlg/UIDialog.xml", L"browser", L"骨架登记"},
    {DialogKind::FaceVerify, L"face_verify", L"人脸验证", L"CFaceVertifyIeDlg", L"FaceVertifyIe/UIDialog.xml", L"browser", L"骨架登记"},
    {DialogKind::FaceCollectUser, L"face_collect_user", L"人脸实名资料收集", L"CFaceCollectUserIeDlg", L"FaceCollectUserIe/UIDialog.xml", L"browser", L"骨架登记"},
    {DialogKind::Geetest, L"geetest", L"极验验证", L"CGeetestDlg", L"ie/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::MessageBox, L"message_box", L"提示框", L"CMessageBox", L"msgbox/UIDialog.xml", L"utility", L"Qt 骨架可打开"},
    {DialogKind::MessageBoxOkCancel, L"message_box_ok_cancel", L"确认取消提示框", L"CMessageBox", L"msgbox/UIDialog_OkCancel.xml", L"utility", L"Qt 骨架可打开"},
    {DialogKind::MessageBoxOkCancelChecked, L"message_box_ok_cancel_checked", L"带勾选提示框", L"CMessageBox", L"msgbox/UIDialog_OKCancelChecked.xml", L"utility", L"Qt 骨架可打开"},
    {DialogKind::Captcha, L"captcha", L"图形验证码", L"CCaptchaDlg", L"captcha/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::ProtectCode, L"protect_code", L"图片验证码保护", L"CProtectCodeDlg", L"Protect_code/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::ProtectA8, L"protect_a8", L"A8 动态码保护", L"CProtectA8Dlg", L"Protect_a8/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::ProtectD6, L"protect_d6", L"D6 密保卡保护", L"CProtectD6Dlg", L"Protect_key/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::ProtectCard, L"protect_card", L"矩阵卡保护", L"CProtectCardDlg", L"protect_card/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::ActiveCode, L"active_code", L"激活码", L"CActiveCodeInfoDlg", L"activeCode/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::StaticLoginCondition, L"static_login_condition", L"静态登录条件", L"CStaticLoginDlg", L"static_login_condition/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::SecurityPhone, L"security_phone", L"安全手机提示", L"CSecurityPhoneDlg", L"security_phone/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::SecurityPhoneStaticLogin, L"security_phone_static_login", L"静态登录安全手机", L"CSecurityPhoneStaticLoginDlg", L"security_phone_static_login/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::GoDownSms, L"go_down_sms", L"安全手机下行短信", L"CGoDownSMSLoginDlg", L"godown_sms_login/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::VoiceVerify, L"voice_verify", L"语音验证码", L"CVoiceInfoDlg", L"voice_vertify/UIDialog.xml", L"challenge", L"骨架登记"},
    {DialogKind::RealNameInfo, L"real_name_info", L"实名信息填写", L"CFcmInfoDlg", L"PlayerInfo_Collect/UIDialog.xml", L"real_name", L"骨架登记"},
    {DialogKind::RealNameConfirm, L"real_name_confirm", L"实名二次确认", L"CFcmInfoTwiceConfirmDlg", L"PlayerInfo_Collect/UIDialog.xml", L"real_name", L"骨架登记"},
    {DialogKind::GuardianInfo, L"guardian_info", L"监护人信息填写", L"CGuardDianInfoDlg", L"guardian_info/UIDialog.xml", L"guardian", L"骨架登记"},
    {DialogKind::GuardianConfirm, L"guardian_confirm", L"监护人信息确认", L"CGuardDianConfirmInfoDlg", L"guardian_confirm/UIDialog.xml", L"guardian", L"骨架登记"},
    {DialogKind::GuardianVoice, L"guardian_voice", L"监护人语音验证", L"CGuardDianVoiceInfoDlg", L"guardian_voice/UIDialog.xml", L"guardian", L"骨架登记"},
    {DialogKind::GuardianCaptcha, L"guardian_captcha", L"监护人验证码", L"CGuardDianCaptchaDlg", L"guardian_captcha/UIDialog.xml", L"guardian", L"骨架登记"},
    {DialogKind::PlayerInfoCollect, L"player_info_collect", L"玩家资料收集", L"CGamePlayerInfoDlg", L"PlayerInfo_Collect/UIDialog.xml", L"real_name", L"骨架登记"},
    {DialogKind::LoginPrompt, L"login_prompt", L"登录提示", L"CLoginPromptDlg", L"login_prompt/UIDialog.xml", L"utility", L"骨架登记"},
    {DialogKind::LoginNotify, L"login_notify", L"登录通知", L"CLoginNotifyDlg", L"login_notify/UIDialog.xml", L"utility", L"骨架登记"},
    {DialogKind::Keyboard, L"keyboard", L"安全键盘", L"CKeyboard", L"keyboard/UIDialog.xml", L"utility", L"骨架登记"},
    {DialogKind::Ime, L"ime", L"游戏内输入法", L"IGImeWindow", L"ime/UIDialog.xml", L"in_game", L"骨架登记"},
    {DialogKind::Tooltip, L"tooltip", L"游戏内提示", L"IGTooltipWnd", L"tooltip/UIDialog.xml", L"in_game", L"骨架登记"},
    {DialogKind::Cursor, L"cursor", L"游戏内光标", L"IGCursor", L"cursor/UIDialog.xml", L"in_game", L"骨架登记"},
    {DialogKind::Promotion, L"promotion", L"推广窗口", L"CPromotionDlg", L"promotion/UIDialog.xml", L"optional", L"骨架登记"},
    {DialogKind::GameSetting, L"game_setting", L"游戏设置", L"CGameSettingDlg", L"game_setting/UIDialog.xml", L"optional", L"骨架登记"},
    {DialogKind::ThirdParty360, L"third_party_360", L"360 登录", L"C360LoginDlg", L"360_login/UIDialog.xml", L"third_party", L"骨架登记"},
    {DialogKind::ThirdPartyMiGu, L"third_party_migu", L"咪咕登录", L"CMiGuLoginDlg", L"migu/UIDialog.xml", L"third_party", L"骨架登记"},
    {DialogKind::ThirdPartyShunWang, L"third_party_shunwang", L"顺网登录", L"CShunWangLoginDlg", L"shunwangie/UIDialog.xml", L"third_party", L"骨架登记"},
    {DialogKind::ThirdPartyXinYou, L"third_party_xinyou", L"新游登录", L"CXinYouLoginDlg", L"xinyouie/UIDialog.xml", L"third_party", L"骨架登记"},
    {DialogKind::ThirdPartyFeiHuoWeb, L"third_party_feihuo_web", L"飞火网页登录", L"CFeiHuoLoginDlg", L"ie/UIDialog.xml", L"third_party", L"骨架登记"},
};

bool sameName(const wchar_t* left, const wchar_t* right)
{
    return left && right && std::wcscmp(left, right) == 0;
}

}

const std::vector<LegacyDialogDescriptor>& legacyDialogs()
{
    return kDialogs;
}

const LegacyDialogDescriptor* findLegacyDialog(DialogKind kind)
{
    for (const LegacyDialogDescriptor& dialog : kDialogs) {
        if (dialog.kind == kind) {
            return &dialog;
        }
    }
    return nullptr;
}

const LegacyDialogDescriptor* findLegacyDialogByName(const wchar_t* stableName)
{
    for (const LegacyDialogDescriptor& dialog : kDialogs) {
        if (sameName(dialog.stableName, stableName)) {
            return &dialog;
        }
    }
    return nullptr;
}

}
