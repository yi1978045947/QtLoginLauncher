#include <cassert>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "command_line.h"
#include "cef_web_bridge.h"
#include "account_history_model.h"
#include "code_key_login_model.h"
#include "dpi_scaler.h"
#include "dx_demo_behavior.h"
#include "legacy_password_auth.h"
#include "protocol_acceptance_model.h"
#include "protocol_browser_launcher.h"
#include "protocol_browser_style.h"
#include "push_message_login_model.h"
#include "quick_login_skin.h"
#include "qt_dpi_config.h"
#include "virtual_keyboard_model.h"
#include "window_embed_style.h"

int main()
{
    using qtlogin::sdologin::parseCommandLineArguments;

    const std::vector<std::wstring> args = {
        L"sdologin.exe",
        L"--pipe=QtLoginRewrite_1",
        L"--standalone-demo",
        L"--mock-cancel",
        L"--mock-success",
        L"--mock-exit-after-show",
        L"--auto-success-ms=50",
        L"--standalone-protocol=privacy",
        L"--standalone-url=https://example.com/",
    };

    const qtlogin::sdologin::SdologinOptions options = parseCommandLineArguments(args);
    assert(options.pipeName == L"QtLoginRewrite_1");
    assert(options.standaloneDemo);
    assert(options.mockCancel);
    assert(options.mockSuccess);
    assert(options.mockExitAfterShow);
    assert(options.autoSuccessMs == 50);
    assert(options.standaloneProtocol == L"privacy");
    assert(options.standaloneUrl == L"https://example.com/");

    const qtlogin::sdologin::SdologinOptions defaults = parseCommandLineArguments({L"sdologin.exe"});
    assert(defaults.pipeName.empty());
    assert(!defaults.standaloneDemo);
    assert(!defaults.mockCancel);
    assert(!defaults.mockSuccess);
    assert(!defaults.mockExitAfterShow);
    assert(defaults.autoSuccessMs < 0);

    const qtlogin::sdologin::SdologinOptions badAuto = parseCommandLineArguments({L"sdologin.exe", L"--auto-success-ms=abc"});
    assert(badAuto.autoSuccessMs < 0);

    const LONG_PTR topLevelStyle = WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    const LONG_PTR embeddedStyle = qtlogin::sdologin::makeEmbeddedChildStyle(topLevelStyle);
    assert((embeddedStyle & WS_CHILD) == WS_CHILD);
    assert((embeddedStyle & WS_CLIPCHILDREN) == WS_CLIPCHILDREN);
    assert((embeddedStyle & WS_CLIPSIBLINGS) == WS_CLIPSIBLINGS);
    assert((embeddedStyle & WS_POPUP) == 0);
    assert((embeddedStyle & WS_CAPTION) == 0);
    assert((embeddedStyle & WS_THICKFRAME) == 0);
    assert((embeddedStyle & WS_SYSMENU) == 0);
    assert((embeddedStyle & WS_MINIMIZEBOX) == 0);
    assert((embeddedStyle & WS_MAXIMIZEBOX) == 0);

    const LONG_PTR embeddedExStyle = qtlogin::sdologin::makeEmbeddedChildExStyle(WS_EX_APPWINDOW | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
    assert((embeddedExStyle & WS_EX_APPWINDOW) == 0);
    assert((embeddedExStyle & WS_EX_LAYERED) == 0);
    assert((embeddedExStyle & WS_EX_TOOLWINDOW) == 0);
    assert(qtlogin::sdologin::shouldMaskEmbeddedWindowBackground(true, true));
    assert(!qtlogin::sdologin::shouldMaskEmbeddedWindowBackground(false, true));
    assert(!qtlogin::sdologin::shouldMaskEmbeddedWindowBackground(true, false));
    assert(qtlogin::sdologin::shouldRemoveEmbeddedWindowAlpha(true, true));
    assert(!qtlogin::sdologin::shouldRemoveEmbeddedWindowAlpha(false, true));
    assert(!qtlogin::sdologin::shouldRemoveEmbeddedWindowAlpha(true, false));
    assert(qtlogin::sdologin::embeddedWindowAlphaCutoff() == 250);
    assert(qtlogin::sdologin::shouldCloseForMissingEmbeddedOwner(true, reinterpret_cast<HWND>(0x1234), false));
    assert(!qtlogin::sdologin::shouldCloseForMissingEmbeddedOwner(true, reinterpret_cast<HWND>(0x1234), true));
    assert(!qtlogin::sdologin::shouldCloseForMissingEmbeddedOwner(false, reinterpret_cast<HWND>(0x1234), false));
    assert(!qtlogin::sdologin::shouldCloseForMissingEmbeddedOwner(true, nullptr, false));
    assert(!qtlogin::sdologin::shouldAllowLoginWindowDrag(true));
    assert(qtlogin::sdologin::shouldAllowLoginWindowDrag(false));

    const auto browserArgs = qtlogin::sdologin::buildProtocolBrowserArguments(
        L"隐私政策",
        L"https://we.sdoprofile.com/common/static/register/public/privacy_game.html");
    assert(browserArgs.size() == 2);
    assert(browserArgs[0] == L"--title=隐私政策");
    assert(browserArgs[1] == L"--url=https://we.sdoprofile.com/common/static/register/public/privacy_game.html");

    qtlogin::sdologin::ProtocolBrowserLaunchOptions browserOptions;
    browserOptions.title = L"用户协议";
    browserOptions.url = L"https://example.test/user.html";
    browserOptions.ownerHwnd = reinterpret_cast<void*>(static_cast<uintptr_t>(0x123456));
    browserOptions.prewarmOnly = true;
    browserOptions.legacyDpiScaleFactor = 1.5;
    browserOptions.agreeDelaySeconds = 5;
    const auto browserArgsWithOwner = qtlogin::sdologin::buildProtocolBrowserArguments(browserOptions);
    assert(browserArgsWithOwner.size() == 6);
    assert(browserArgsWithOwner[0] == L"--title=用户协议");
    assert(browserArgsWithOwner[1] == L"--url=https://example.test/user.html");
    assert(browserArgsWithOwner[2] == L"--owner-hwnd=1193046");
    assert(browserArgsWithOwner[3] == L"--dpi-scale=1.5");
    assert(browserArgsWithOwner[4] == L"--agree-delay-seconds=5");
    assert(browserArgsWithOwner[5] == L"--prewarm");

    assert(qtlogin::sdologin::shouldRequireInitialProtocolReview(-1));
    assert(!qtlogin::sdologin::shouldRequireInitialProtocolReview(1));
    assert(!qtlogin::sdologin::initialProtocolCheckboxChecked(true));
    assert(qtlogin::sdologin::initialProtocolCheckboxChecked(false));
    assert(qtlogin::sdologin::shouldOpenInitialPrivacyProtocol(true, false, true, false));
    assert(!qtlogin::sdologin::shouldOpenInitialPrivacyProtocol(true, true, true, false));
    assert(!qtlogin::sdologin::shouldOpenInitialPrivacyProtocol(true, false, false, false));
    assert(!qtlogin::sdologin::shouldOpenInitialPrivacyProtocol(false, false, true, false));
    assert(!qtlogin::sdologin::shouldOpenInitialPrivacyProtocol(true, false, true, true));
    assert(qtlogin::sdologin::normalizedProtocolAgreeDelaySeconds(5) == 5);
    assert(qtlogin::sdologin::normalizedProtocolAgreeDelaySeconds(0) == 5);
    assert(qtlogin::sdologin::shouldRequireProtocolReviewAfterServerConfig(-1, false, -1));
    assert(qtlogin::sdologin::shouldRequireProtocolReviewAfterServerConfig(3, true, 4));
    assert(!qtlogin::sdologin::shouldRequireProtocolReviewAfterServerConfig(4, true, 4));
    assert(qtlogin::sdologin::acceptedPrivacyPolicyVersionForStorage(true, 7) == 7);
    assert(qtlogin::sdologin::acceptedPrivacyPolicyVersionForStorage(true, -1) == -1);
    assert(qtlogin::sdologin::acceptedPrivacyPolicyVersionForStorage(false, 7) == -1);
    assert(qtlogin::sdologin::shouldPersistInitialProtocolAcceptance(true, 0, 7));
    assert(!qtlogin::sdologin::shouldPersistInitialProtocolAcceptance(true, 10, 7));
    assert(!qtlogin::sdologin::shouldPersistInitialProtocolAcceptance(false, 0, 7));
    assert(!qtlogin::sdologin::shouldPersistInitialProtocolAcceptance(true, 0, -1));

    const qtlogin::sdologin::SkinButtonSpec connectButton = qtlogin::sdologin::sharedConnectButtonSpec();
    assert(connectButton.normalImage == "common/btn_login_n.png");
    assert(connectButton.hoverImage == "common/btn_login_c.png");
    assert(connectButton.pressedImage == "common/btn_login_c.png");
    assert(connectButton.disabledImage == "common/btn_login_c.png");
    assert(connectButton.x == 281);
    assert(connectButton.y == 58);
    assert(connectButton.width == 104);
    assert(connectButton.height == 52);

    const qtlogin::sdologin::QuickLoginSkinSpec quickSkin = qtlogin::sdologin::quickLoginSkinSpec();
    assert(quickSkin.actionButton.normalImage == "common/btn_login_n.png");
    assert(quickSkin.actionButton.hoverImage == connectButton.hoverImage);
    assert(quickSkin.actionButton.pressedImage == "common/btn_login_c.png");
    assert(quickSkin.actionButton.disabledImage == "common/btn_login_c.png");
    assert(quickSkin.actionButton.x == connectButton.x);
    assert(quickSkin.actionButton.y == connectButton.y);
    assert(quickSkin.actionButton.width == connectButton.width);
    assert(quickSkin.actionButton.height == connectButton.height);
    assert(quickSkin.retryButton.normalImage == "push_message/6_a.png");
    assert(quickSkin.retryButton.hoverImage == "push_message/6_b.png");
    assert(quickSkin.retryButton.pressedImage == "push_message/6_c.png");
    assert(quickSkin.retryButton.disabledImage == "push_message/6_d.png");
    assert(quickSkin.retryCountdownSeconds == 6);

    const qtlogin::sdologin::MainLoginTabSpec tabSpec = qtlogin::sdologin::mainLoginTabSpec();
    assert(tabSpec.passwordTab == qtlogin::sdologin::LoginPageSlot::Password);
    assert(tabSpec.oneClickTab == qtlogin::sdologin::LoginPageSlot::PushMessage);
    assert(tabSpec.qrTab == qtlogin::sdologin::LoginPageSlot::QrCode);
    assert(tabSpec.quickShortcut == qtlogin::sdologin::LoginPageSlot::PushMessage);
    assert(qtlogin::sdologin::loginPageIndex(qtlogin::sdologin::LoginPageSlot::PushMessage) == 1);
    assert(qtlogin::sdologin::loginPageIndex(qtlogin::sdologin::LoginPageSlot::SmsCode) == 3);

    const std::string protocolStyle = qtlogin::sdologin::protocolBrowserDialogStyleSheet();
    assert(protocolStyle.find("QDialog{background:#ffffff;}") != std::string::npos);
    assert(protocolStyle.find("#3c2418") == std::string::npos);
    assert(qtlogin::sdologin::protocolBrowserLoadingBackgroundColor() == "#ffffff");
    assert(!qtlogin::sdologin::shouldShowProtocolLoadError(-3));
    assert(qtlogin::sdologin::shouldShowProtocolLoadError(-105));
    assert(qtlogin::sdologin::protocolBrowserQtFallbackStatusText() == "Qt network protocol rendering");
    assert(qtlogin::sdologin::cefBridgeObjectName() == std::string("qtlogin"));
    assert(qtlogin::sdologin::cefBridgePostMessageFunctionName() == std::string("postMessage"));
    assert(qtlogin::sdologin::cefBridgeOnMessageFunctionName() == std::string("onMessage"));
    assert(qtlogin::sdologin::cefBridgeWebToNativeMessageName() == std::string("qtlogin.webToNative"));
    assert(qtlogin::sdologin::cefBridgeNativeToWebMessageName() == std::string("qtlogin.nativeToWeb"));
    assert(qtlogin::sdologin::shouldLoadProtocolUrlWithQtFallback("https://we.sdoprofile.com/common/static/register/public/user_agreements.html"));
    assert(qtlogin::sdologin::shouldLoadProtocolUrlWithQtFallback("http://example.test/protocol.html"));
    assert(!qtlogin::sdologin::shouldLoadProtocolUrlWithQtFallback(""));
    assert(!qtlogin::sdologin::shouldLoadProtocolUrlWithQtFallback("about:blank"));
    assert(qtlogin::sdologin::shouldLoadSdoAgreementContentJsonp("https://we.sdoprofile.com/common/static/register/public/user_agreements.html"));
    assert(qtlogin::sdologin::shouldLoadSdoAgreementContentJsonp("https://we.sdoprofile.com/common/static/register/public/user_agreements.html?appId=9001&scene=login"));
    assert(!qtlogin::sdologin::shouldLoadSdoAgreementContentJsonp("https://we.sdoprofile.com/common/static/register/public/privacy_game.html"));
    assert(qtlogin::sdologin::sdoAgreementContentJsonpUrl("https://we.sdoprofile.com/common/static/register/public/user_agreements.html") == "https://utility.sdo.com/agreement/content?appId=undefined&fn_callback=callback");
    assert(qtlogin::sdologin::sdoAgreementContentJsonpUrl("https://we.sdoprofile.com/common/static/register/public/user_agreements.html?appId=9001&scene=login") == "https://utility.sdo.com/agreement/content?appId=9001&scene=login&fn_callback=callback");
    assert(qtlogin::sdologin::shouldLoadSdoViolationNewsContent("https://mxd.web.sdo.com/web7/news/violation.html"));
    assert(!qtlogin::sdologin::shouldLoadSdoViolationNewsContent("https://we.sdoprofile.com/common/static/register/public/user_agreements.html"));
    assert(qtlogin::sdologin::sdoViolationNewsContentUrl("https://mxd.web.sdo.com/web7/news/violation.html") == "https://mxd.web.sdo.com/web7/Handler/NewsContent.ashx?id=350555");
    assert(qtlogin::sdologin::resolveProtocolResourceUrl(
        "https://we.sdoprofile.com/common/static/register/public/privacy_game.html",
        "images/icon.png") == "https://we.sdoprofile.com/common/static/register/public/images/icon.png");
    assert(qtlogin::sdologin::resolveProtocolResourceUrl(
        "https://we.sdoprofile.com/common/static/register/public/privacy_game.html",
        "/common/static/register/public/images/icon.png") == "https://we.sdoprofile.com/common/static/register/public/images/icon.png");
    assert(qtlogin::sdologin::resolveProtocolResourceUrl(
        "https://we.sdoprofile.com/common/static/register/public/privacy_game.html",
        "//gskd.sdoprofile.com/sq_protocol/wxcode.png") == "https://gskd.sdoprofile.com/sq_protocol/wxcode.png");
    assert(qtlogin::sdologin::resolveProtocolResourceUrl(
        "https://we.sdoprofile.com/common/static/register/public/privacy_game.html",
        "https://login.sdo.com/sdo/Login/LoginSDO.php") == "https://login.sdo.com/sdo/Login/LoginSDO.php");
    assert(qtlogin::sdologin::shouldOpenProtocolLinkExternally("https://login.sdo.com/sdo/Login/LoginSDO.php"));
    assert(qtlogin::sdologin::shouldOpenProtocolLinkExternally("http://example.test/help"));
    assert(!qtlogin::sdologin::shouldOpenProtocolLinkExternally("#section1"));
    assert(!qtlogin::sdologin::shouldOpenProtocolLinkExternally("javascript:void(0)"));
    assert(qtlogin::sdologin::protocolExternalLinkTarget(
        "https://we.sdoprofile.com/common/static/register/public/privacy_game.html",
        "privacy_sdk.html") == "https://we.sdoprofile.com/common/static/register/public/privacy_sdk.html");
    assert(qtlogin::sdologin::protocolExternalLinkTarget(
        "https://we.sdoprofile.com/common/static/register/public/privacy_game.html",
        "https://login.sdo.com/sdo/Login/LoginSDO.php") == "https://login.sdo.com/sdo/Login/LoginSDO.php");
    assert(qtlogin::sdologin::protocolExternalLinkTarget(
        "https://we.sdoprofile.com/common/static/register/public/privacy_game.html",
        "#section1").empty());
    assert(qtlogin::sdologin::protocolExternalLinkTarget(
        "https://we.sdoprofile.com/common/static/register/public/privacy_game.html",
        "javascript:void(0)").empty());
    const auto imageUrls = qtlogin::sdologin::protocolImageResourceUrls(
        "https://we.sdoprofile.com/common/static/register/public/privacy_game.html",
        "<img src=\"images/wxcode.png\"><div style=\"background-image:url('/common/static/register/public/images/logo.png')\"></div>"
        "<img src=\"data:image/png;base64,abc\"><img src=\"//gskd.sdoprofile.com/sq_protocol/wxcode.png\">");
    assert(imageUrls.size() == 3);
    assert(std::find(imageUrls.begin(), imageUrls.end(), "https://we.sdoprofile.com/common/static/register/public/images/wxcode.png") != imageUrls.end());
    assert(std::find(imageUrls.begin(), imageUrls.end(), "https://we.sdoprofile.com/common/static/register/public/images/logo.png") != imageUrls.end());
    assert(std::find(imageUrls.begin(), imageUrls.end(), "https://gskd.sdoprofile.com/sq_protocol/wxcode.png") != imageUrls.end());
    const std::string rewrittenProtocolHtml = qtlogin::sdologin::rewriteProtocolResourceUrls(
        "<img src=\"https://gskd.sdoprofile.com/sq_protocol/wxcode.png\">",
        {{"https://gskd.sdoprofile.com/sq_protocol/wxcode.png", "file:///C:/cache/wxcode.png"}});
    assert(rewrittenProtocolHtml.find("file:///C:/cache/wxcode.png") != std::string::npos);
    assert(rewrittenProtocolHtml.find("https://gskd.sdoprofile.com/sq_protocol/wxcode.png") == std::string::npos);
    assert(qtlogin::sdologin::qtLoginHighDpiScalingEnvValue() == std::string("0"));
    assert(qtlogin::sdologin::qtLoginAutoScreenScaleFactorEnvValue() == std::string("0"));
    assert(qtlogin::sdologin::qtLoginShouldForce96Dpi());

    assert(qtlogin::sdologin::legacyDpiScaleFactorFromDpi(96) == 1.0);
    assert(qtlogin::sdologin::legacyDpiScaleFactorFromDpi(120) == 1.25);
    assert(qtlogin::sdologin::legacyDpiScaleFactorFromDpi(144) == 1.5);
    assert(qtlogin::sdologin::legacyDpiScaleFactorFromPercent(125) == 1.25);
    assert(qtlogin::sdologin::chooseLegacyDpiScaleFactor(0.0, 0.0, 144) == 1.5);
    assert(qtlogin::sdologin::chooseLegacyDpiScaleFactor(1.25, 0.0, 144) == 1.25);
    assert(qtlogin::sdologin::chooseLegacyDpiScaleFactor(0.0, 1.25, 144) == 1.25);
    assert(qtlogin::sdologin::chooseLegacyDpiScaleFactor(0.0, 0.0, 0) == 1.0);
    assert(qtlogin::sdologin::scaleLegacyPixel(442, 1.25) == 553);
    const qtlogin::sdologin::LegacyRect scaledTab =
        qtlogin::sdologin::scaleLegacyRect({49, 204, 108, 44}, 1.25);
    assert(scaledTab.x == 61);
    assert(scaledTab.y == 255);
    assert(scaledTab.width == 135);
    assert(scaledTab.height == 55);
    const std::string scaledStyle = qtlogin::sdologin::scaleLegacyStyleSheetPixels(
        "QLabel{font:12px 'Microsoft YaHei';padding-left:8px;border:1px solid #fff;} QLineEdit{padding-right:10px;}",
        1.5);
    assert(scaledStyle.find("font:18px") != std::string::npos);
    assert(scaledStyle.find("padding-left:12px") != std::string::npos);
    assert(scaledStyle.find("border:2px") != std::string::npos);
    assert(scaledStyle.find("padding-right:15px") != std::string::npos);
    const std::string scaledCheckStyle = qtlogin::sdologin::scaleLegacyStyleSheetPixels(
        "QCheckBox::indicator{width:18px;height:15px;border:0;border-image:url(a);}",
        1.5);
    assert(scaledCheckStyle.find("width:27px") != std::string::npos);
    assert(scaledCheckStyle.find("height:23px") != std::string::npos);

    const qtlogin::samples::DxDemoBehavior dxBehavior = qtlogin::samples::defaultDxDemoBehavior();
    assert(dxBehavior.windowWidth == 1280);
    assert(dxBehavior.windowHeight == 720);
    assert(dxBehavior.autoShowLogin);
    const qtlogin::samples::DxDemoRect centered = qtlogin::samples::centeredDxDemoWindowRect(1920, 1080, dxBehavior);
    assert(centered.x == 320);
    assert(centered.y == 180);
    assert(centered.width == 1280);
    assert(centered.height == 720);
    const std::wstring successCallbackMessage =
        qtlogin::samples::dxDemoLoginCallbackMessage(0, L"10001", L"session-abc");
    assert(successCallbackMessage.find(L"Login callback success") != std::wstring::npos);
    assert(successCallbackMessage.find(L"userid=10001") != std::wstring::npos);
    assert(successCallbackMessage.find(L"sessionid=session-abc") != std::wstring::npos);
    const std::wstring failedCallbackMessage =
        qtlogin::samples::dxDemoLoginCallbackMessage(-1, nullptr, nullptr);
    assert(failedCallbackMessage.find(L"Login callback error") != std::wstring::npos);
    assert(failedCallbackMessage.find(L"errorCode=-1") != std::wstring::npos);
    const auto& dxButtons = qtlogin::samples::dxDemoApiButtons();
    assert(dxButtons.size() >= 20);
    assert(dxButtons[0].action == qtlogin::samples::DxDemoApiAction::ShowLoginDialog);
    assert(std::wstring(dxButtons[0].text) == L"ShowLoginDialog");
    assert(dxButtons[1].action == qtlogin::samples::DxDemoApiAction::SetLoginMode);
    assert(dxButtons[2].action == qtlogin::samples::DxDemoApiAction::SetOwnerWindow);
    assert(dxButtons[3].action == qtlogin::samples::DxDemoApiAction::MoveLoginDialog);
    assert(dxButtons[7].action == qtlogin::samples::DxDemoApiAction::GetTicketForAppId);
    assert(dxButtons[8].action == qtlogin::samples::DxDemoApiAction::SetDoubleLogin);
    assert(dxButtons[9].action == qtlogin::samples::DxDemoApiAction::SetDpi);
    assert(dxButtons[14].action == qtlogin::samples::DxDemoApiAction::OpenFaceVerify);
    assert(dxButtons[15].action == qtlogin::samples::DxDemoApiAction::OpenFaceCollectMsg);
    assert(dxButtons[16].action == qtlogin::samples::DxDemoApiAction::GhomePay);
    assert(dxButtons[17].action == qtlogin::samples::DxDemoApiAction::GhomeGetChannelCode);
    assert(dxButtons.back().action == qtlogin::samples::DxDemoApiAction::IsdolOpenWindow);
    const qtlogin::samples::DxDemoPoint rightLogin =
        qtlogin::samples::dxDemoLoginDialogPosition(1280, 720, 1.5);
    assert(rightLogin.x >= 590);
    assert(rightLogin.y >= 32);
    const qtlogin::samples::DxDemoPoint smallLogin =
        qtlogin::samples::dxDemoLoginDialogPosition(800, 600, 1.0);
    assert(smallLogin.x >= 334);
    assert(smallLogin.x + 442 <= 800);

    const auto protocolRequired = qtlogin::sdologin::validatePasswordLoginRequest({L"user", L"pwd", false});
    assert(protocolRequired.code == qtlogin::sdologin::PasswordLoginValidationCode::ProtocolRequired);
    const auto emptyAccount = qtlogin::sdologin::validatePasswordLoginRequest({L"  \t", L"pwd", true});
    assert(emptyAccount.code == qtlogin::sdologin::PasswordLoginValidationCode::EmptyAccount);
    const auto emptyPassword = qtlogin::sdologin::validatePasswordLoginRequest({L"user", L"", true});
    assert(emptyPassword.code == qtlogin::sdologin::PasswordLoginValidationCode::EmptyPassword);
    const auto validPassword = qtlogin::sdologin::validatePasswordLoginRequest({L"  player001  ", L"pwd", true});
    assert(validPassword.code == qtlogin::sdologin::PasswordLoginValidationCode::Ok);
    assert(validPassword.trimmedAccount == L"player001");

    const std::string rsaPasswordBase64 = qtlogin::sdologin::base64EncodeForLegacyAuth("rsa-password");
    const auto payload = qtlogin::sdologin::makeLegacyStaticLoginPayload(
        L"  player001  ",
        rsaPasswordBase64,
        "dynamic-key",
        1,
        false,
        [](const std::string& plain, const std::string& key) {
            return std::string("enc(") + plain + "," + key + ")";
        });
    assert(payload.inputUserId == "enc(player001,dynamic-key)");
    assert(payload.password == "enc(rsa-password,dynamic-key)");
    assert(payload.inputUserType == 1);
    assert(payload.keepLoginFlag == 0);
    assert(payload.scene == "pc_login");
    assert(qtlogin::sdologin::passwordLoginScene(true) == "xp_login");
    assert(qtlogin::sdologin::mapLegacySdoBaseResultCode(-10130123) == -10524123);
    assert(qtlogin::sdologin::mapLegacySdoBaseResultCode(-10130200) == -10524200);
    assert(qtlogin::sdologin::mapLegacySdoBaseResultCode(-1) == -1);

    const auto qrPending = qtlogin::sdologin::makeCodeKeyLoginResultFromLegacyCallback(
        qtlogin::sdologin::kCodeKeyContinuePollingErrorCode,
        "",
        "",
        "",
        "",
        "",
        "");
    assert(qrPending.continuePolling);
    assert(!qrPending.scanned);
    assert(qtlogin::sdologin::shouldContinueCodeKeyPolling(qrPending));
    assert(qtlogin::sdologin::codeKeyStateForResult(qrPending) == qtlogin::sdologin::CodeKeyLoginState::CodeReady);

    const auto qrScanned = qtlogin::sdologin::makeCodeKeyLoginResultFromLegacyCallback(
        qtlogin::sdologin::kCodeKeyContinuePollingErrorCode,
        "",
        "",
        "",
        "",
        "",
        "1");
    assert(qrScanned.continuePolling);
    assert(qrScanned.scanned);
    assert(qtlogin::sdologin::codeKeyStateForResult(qrScanned) == qtlogin::sdologin::CodeKeyLoginState::Scanned);

    const auto qrExpired = qtlogin::sdologin::makeCodeKeyLoginResultFromLegacyCallback(
        qtlogin::sdologin::kCodeKeyExpiredErrorCode,
        "expired",
        "",
        "",
        "",
        "",
        "");
    assert(qrExpired.expired);
    assert(qtlogin::sdologin::codeKeyStateForResult(qrExpired) == qtlogin::sdologin::CodeKeyLoginState::Expired);

    const auto qrSuccess = qtlogin::sdologin::makeCodeKeyLoginResultFromLegacyCallback(
        0,
        "",
        "10001",
        "ticket-value",
        "tgt-value",
        "display-name",
        "");
    assert(qrSuccess.success);
    assert(qrSuccess.sndaId == "10001");
    assert(qrSuccess.ticket == "ticket-value");
    assert(qrSuccess.sessionId == "tgt-value");
    assert(qtlogin::sdologin::codeKeyStateForResult(qrSuccess) == qtlogin::sdologin::CodeKeyLoginState::Success);
    assert(qtlogin::sdologin::normalizedCodeKeyRefreshClickIntervalMs(1500) == 1500);
    assert(qtlogin::sdologin::normalizedCodeKeyRefreshClickIntervalMs(0) == 1500);
    assert(qtlogin::sdologin::normalizedCodeKeyRefreshClickIntervalMs(-1) == 1500);
    assert(qtlogin::sdologin::shouldAllowCodeKeyRefreshClick(-1, 0, 1500));
    assert(qtlogin::sdologin::shouldAllowCodeKeyRefreshClick(1000, 2499, 1500) == false);
    assert(qtlogin::sdologin::shouldAllowCodeKeyRefreshClick(1000, 2500, 1500));

    const auto pushProtocolRequired = qtlogin::sdologin::validatePushMessageLoginRequest({L"user", false});
    assert(pushProtocolRequired.code == qtlogin::sdologin::PushMessageLoginValidationCode::ProtocolRequired);
    const auto pushEmptyAccount = qtlogin::sdologin::validatePushMessageLoginRequest({L"  \t", true});
    assert(pushEmptyAccount.code == qtlogin::sdologin::PushMessageLoginValidationCode::EmptyAccount);
    const auto pushValid = qtlogin::sdologin::validatePushMessageLoginRequest({L"  player001  ", true});
    assert(pushValid.code == qtlogin::sdologin::PushMessageLoginValidationCode::Ok);
    assert(pushValid.trimmedAccount == L"player001");
    const auto pushMobileOnlyInvalid = qtlogin::sdologin::validatePushMessageLoginRequest({L"abc123", true, true});
    assert(pushMobileOnlyInvalid.code == qtlogin::sdologin::PushMessageLoginValidationCode::InvalidMobileAccount);
    const auto pushMobileOnlyValid = qtlogin::sdologin::validatePushMessageLoginRequest({L"+8613800138000", true, true});
    assert(pushMobileOnlyValid.code == qtlogin::sdologin::PushMessageLoginValidationCode::Ok);

    assert(qtlogin::sdologin::pushMessageLoginScene(false) == "pc_pushmsglogin");
    assert(qtlogin::sdologin::pushMessageLoginScene(true) == "xp_pushmsglogin");
    assert(qtlogin::sdologin::normalizedPushMessageRetryCountdownSeconds(6) == 6);
    assert(qtlogin::sdologin::normalizedPushMessageRetryCountdownSeconds(0) == 6);
    assert(qtlogin::sdologin::shouldIssuePushMessageSendAfterCancel(false));
    assert(!qtlogin::sdologin::shouldIssuePushMessageSendAfterCancel(true));
    assert(qtlogin::sdologin::shouldWaitForPushMessageCancelCallback(0));
    assert(!qtlogin::sdologin::shouldWaitForPushMessageCancelCallback(-10130002));
    assert(qtlogin::sdologin::shouldContinuePushMessageAfterCancelCallback(0));
    assert(qtlogin::sdologin::shouldContinuePushMessageAfterCancelCallback(-10242301));
    assert(qtlogin::sdologin::shouldApplyPushMessageButtonGeometryForSkinChange(true));
    assert(!qtlogin::sdologin::shouldApplyPushMessageButtonGeometryForSkinChange(false));

    const std::vector<qtlogin::config::ConfigManager::UserAccountEntry> accountHistory = {
        {L"latest-user", 0},
        {L"old-user", 1},
    };
    assert(qtlogin::sdologin::preferredAccountHistoryText(accountHistory) == L"latest-user");
    assert(qtlogin::sdologin::preferredAccountHistoryText({}).empty());
    const auto accountHistoryAfterRemove = qtlogin::sdologin::removeAccountHistoryEntry(accountHistory, L"LATEST-USER");
    assert(accountHistoryAfterRemove.size() == 1);
    assert(accountHistoryAfterRemove.front().name == L"old-user");
    assert(qtlogin::sdologin::accountInputTextAfterHistoryRemoval(L"latest-user", accountHistoryAfterRemove) == L"latest-user");
    assert(qtlogin::sdologin::accountInputTextAfterHistoryRemoval(L"", accountHistoryAfterRemove) == L"old-user");
    assert(qtlogin::sdologin::shouldRefreshAccountHistoryUiAfterRecord(false));
    assert(!qtlogin::sdologin::shouldRefreshAccountHistoryUiAfterRecord(true));
    assert(qtlogin::sdologin::shouldQueueAccountHistoryRemoveFromPopupClick());
    assert(qtlogin::sdologin::accountHistoryPopupFontPixelSize(1.0) == 16);
    assert(qtlogin::sdologin::accountHistoryPopupFontPixelSize(1.5) == 24);
    assert(qtlogin::sdologin::accountHistoryPopupFontPixelSize(0.0) == 16);

    const auto pushPending = qtlogin::sdologin::makePushMessageLoginResultFromLegacyCallback(
        qtlogin::sdologin::kPushMessageContinuePollingErrorCode,
        "",
        "",
        "",
        "",
        "");
    assert(pushPending.continuePolling);
    assert(qtlogin::sdologin::shouldContinuePushMessagePolling(pushPending));
    assert(qtlogin::sdologin::pushMessageStateForLoginResult(pushPending) == qtlogin::sdologin::PushMessageLoginState::WaitingForMobileConfirm);

    const auto pushCanceled = qtlogin::sdologin::makePushMessageLoginResultFromLegacyCallback(
        qtlogin::sdologin::kPushMessageMobileCanceledErrorCode,
        "mobile canceled",
        "",
        "",
        "",
        "");
    assert(pushCanceled.userCanceledOnMobile);
    assert(!qtlogin::sdologin::shouldContinuePushMessagePolling(pushCanceled));
    assert(qtlogin::sdologin::pushMessageStateForLoginResult(pushCanceled) == qtlogin::sdologin::PushMessageLoginState::Failed);

    const auto pushSuccess = qtlogin::sdologin::makePushMessageLoginResultFromLegacyCallback(
        0,
        "",
        "10001",
        "ticket-value",
        "tgt-value",
        "display-name");
    assert(pushSuccess.success);
    assert(pushSuccess.sndaId == "10001");
    assert(pushSuccess.ticket == "ticket-value");
    assert(pushSuccess.sessionId == "tgt-value");
    assert(pushSuccess.inputUserId == "display-name");
    assert(qtlogin::sdologin::pushMessageStateForLoginResult(pushSuccess) == qtlogin::sdologin::PushMessageLoginState::Success);

    const auto keys = qtlogin::sdologin::legacyKeyboardKeys();
    assert(keys.size() == 47);
    assert(keys.front().normal == '`');
    assert(keys.front().shifted == '~');
    assert(keys[14].normal == 'q');
    assert(keys[14].shifted == 'Q');
    assert(keys.back().normal == '/');
    assert(keys.back().shifted == '?');
    const auto order = qtlogin::sdologin::randomizedLegacyKeyboardOrder(1234);
    assert(order.size() == keys.size());
    assert(qtlogin::sdologin::keyboardOrderKeepsLegacyRows(order));
    const qtlogin::sdologin::LegacyRect keyboardSize = qtlogin::sdologin::legacyKeyboardWindowRect(1.5);
    assert(keyboardSize.width == 734);
    assert(keyboardSize.height == 312);
    const qtlogin::sdologin::LegacyRect firstKey = qtlogin::sdologin::legacyKeyboardKeyRectForPosition(0, 1.5);
    assert(firstKey.x == 27);
    assert(firstKey.y == 27);
    assert(firstKey.width == 39);
    assert(firstKey.height == 33);
    const qtlogin::sdologin::LegacyRect keyboardHint =
        qtlogin::sdologin::legacyKeyboardControlRect(qtlogin::sdologin::LegacyKeyboardControl::Hint, 1.5);
    assert(keyboardHint.x == 15);
    assert(keyboardHint.y == 246);
    assert(keyboardHint.width == 390);
    assert(keyboardHint.height == 33);

    std::cout << "sdologin_tests passed\n";
    return 0;
}
