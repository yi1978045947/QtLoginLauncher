#include "login_window.h"

#include <QApplication>
#include <QBitmap>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QHBoxLayout>
#include <QLabel>
#include <QImage>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QMetaObject>
#include <QPointer>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QtGlobal>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>
#include <sstream>
#include <utility>
#include <windows.h>

#include "account_history_combo.h"
#include "account_history_model.h"
#include "code_key_login_panel.h"
#include "config_manager.h"
#include "dpi_scaler.h"
#include "legacy_dialog_registry.h"
#include "logger.h"
#include "process_util.h"
#include "protocol_acceptance_model.h"
#include "protocol_browser_launcher.h"
#include "push_message_login_panel.h"
#include "quick_login_skin.h"
#include "virtual_keyboard_dialog.h"
#include "window_embed_style.h"

namespace qtlogin::sdologin {

namespace {

QString fileUrl(const QString& path)
{
    QString normalized = QDir::fromNativeSeparators(path);
    return QStringLiteral("url(\"%1\")").arg(normalized);
}

QString configuredExternalUrl(const std::wstring& configured, const QString& fallback)
{
    if (configured.empty()) {
        return fallback;
    }
    return QString::fromWCharArray(configured.c_str(), static_cast<int>(configured.size()));
}

bool openExternalBrowserUrl(const QString& url)
{
    return QDesktopServices::openUrl(QUrl(url));
}

bool openDaoyuHomePage(const LoginWindowContext& context)
{
    return openExternalBrowserUrl(configuredExternalUrl(
        context.daoyuHomeUrl,
        QStringLiteral("https://www.daoyu8.com/#/")));
}

bool openForgotPasswordPage(const LoginWindowContext& context)
{
    return openExternalBrowserUrl(configuredExternalUrl(
        context.forgotPasswordUrl,
        QStringLiteral("https://i.sdo.com/index/findPassword")));
}

void setButtonImages(QPushButton* button, const QString& normal, const QString& hover = QString(), const QString& pressed = QString(), const QString& checkedOrDisabled = QString())
{
    QString style = QStringLiteral("QPushButton{border:0;border-image:%1;color:#ffeddc;background:transparent;font:bold 12px 'Microsoft YaHei';}").arg(fileUrl(normal));
    if (!hover.isEmpty()) {
        style += QStringLiteral("QPushButton:hover{border-image:%1;}").arg(fileUrl(hover));
    }
    if (!pressed.isEmpty()) {
        style += QStringLiteral("QPushButton:pressed{border-image:%1;}").arg(fileUrl(pressed));
    }
    if (!checkedOrDisabled.isEmpty()) {
        const QString imageUrl = fileUrl(checkedOrDisabled);
        style += QStringLiteral("QPushButton:checked{border-image:%1;}QPushButton:disabled{border-image:%1;}").arg(imageUrl);
    }
    button->setStyleSheet(style);
}

void setImageLabel(QLabel* label, const QString& path)
{
    QPixmap pixmap(path);
    if (!pixmap.isNull()) {
        label->setPixmap(pixmap);
        label->setScaledContents(true);
    }
}

QString editStyle(const QString& normal, const QString& focus)
{
    return QStringLiteral(
        "QLineEdit{border:0;border-image:%1;padding-left:8px;padding-right:8px;color:#ffeddc;placeholder-text-color:#e8c6a3;background:transparent;selection-background-color:#7b4b23;font:12px 'Microsoft YaHei';}"
        "QLineEdit:focus{border-image:%2;}")
        .arg(fileUrl(normal), fileUrl(focus));
}

QString checkBoxIndicatorStyle(const QString& normal, const QString& checked)
{
    return QStringLiteral(
        "QCheckBox{background:transparent;spacing:0px;}"
        "QCheckBox::indicator{width:18px;height:15px;border:0;border-image:%1;}"
        "QCheckBox::indicator:checked{border-image:%2;}").arg(fileUrl(normal), fileUrl(checked));
}

void applyButtonSpec(QPushButton* button, const SkinButtonSpec& spec, const std::function<QString(const std::string&)>& assetPath)
{
    button->setGeometry(spec.x, spec.y, spec.width, spec.height);
    setButtonImages(button,
        assetPath(spec.normalImage),
        assetPath(spec.hoverImage),
        assetPath(spec.pressedImage),
        assetPath(spec.disabledImage));
}

bool isUnitScale(double scaleFactor)
{
    return std::fabs(scaleFactor - 1.0) < 0.001;
}

QRect scaleQtRect(const QRect& rect, double scaleFactor)
{
    const LegacyRect scaled = scaleLegacyRect(
        {rect.x(), rect.y(), rect.width(), rect.height()},
        scaleFactor);
    return QRect(scaled.x, scaled.y, scaled.width, scaled.height);
}

QString scaleStyleSheetPixels(const QString& styleSheet, double scaleFactor)
{
    if (isUnitScale(scaleFactor)) {
        return styleSheet;
    }
    return QString::fromStdString(scaleLegacyStyleSheetPixels(styleSheet.toStdString(), scaleFactor));
}

void scaleLegacyWidgetTree(QWidget* root, double scaleFactor)
{
    if (!root || isUnitScale(scaleFactor)) {
        return;
    }

    const auto children = root->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* child : children) {
        if (!qobject_cast<QStackedWidget*>(child->parentWidget())) {
            child->setGeometry(scaleQtRect(child->geometry(), scaleFactor));
        }
        scaleLegacyWidgetTree(child, scaleFactor);
    }
}

void scaleLegacyStyleSheetTree(QWidget* root, double scaleFactor)
{
    if (!root || isUnitScale(scaleFactor)) {
        return;
    }

    const QString styleSheet = root->styleSheet();
    if (!styleSheet.isEmpty()) {
        root->setStyleSheet(scaleStyleSheetPixels(styleSheet, scaleFactor));
    }

    const auto children = root->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* child : children) {
        scaleLegacyStyleSheetTree(child, scaleFactor);
    }
}

void scaleLegacyFontTree(QWidget* root, double scaleFactor)
{
    if (!root || isUnitScale(scaleFactor)) {
        return;
    }

    QFont font = root->font();
    font.setPixelSize(scaleLegacyPixel(12, scaleFactor));
    root->setFont(font);

    const auto children = root->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* child : children) {
        scaleLegacyFontTree(child, scaleFactor);
    }
}

QPoint globalMousePosition(QMouseEvent* event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}

std::wstring hwndText(HWND hwnd)
{
    std::wostringstream out;
    out << L"0x" << std::hex << reinterpret_cast<uintptr_t>(hwnd);
    return out.str();
}

std::wstring rectText(const RECT& rect)
{
    std::wostringstream out;
    out << (rect.right - rect.left) << L"x" << (rect.bottom - rect.top)
        << L" at " << rect.left << L"," << rect.top;
    return out.str();
}

SdoBaseAuthConfig authConfigFromContext(const LoginWindowContext& context)
{
    SdoBaseAuthConfig config;
    config.udpPassport = wideToAnsiForLegacyAuth(context.udpPassport);
    config.tcpPassport = wideToAnsiForLegacyAuth(context.tcpPassport);
    config.productVersion = wideToAnsiForLegacyAuth(context.appVersion);
    if (config.productVersion.empty()) {
        config.productVersion = "1.0.0";
    }
    config.appId = context.appId;
    config.areaId = context.areaId;
    config.groupId = context.groupId;
    config.verifyCert = context.verifyCert;
    return config;
}

bool hasUsableServerClientConfig(const LoginWindowContext& context)
{
    return context.hasServerClientConfig && context.serverClientConfig.parseOk;
}

qtlogin::config::LoginMethod methodForSlot(LoginPageSlot slot)
{
    switch (slot) {
    case LoginPageSlot::Password:
        return qtlogin::config::LoginMethod::Password;
    case LoginPageSlot::PushMessage:
        return qtlogin::config::LoginMethod::PushMessage;
    case LoginPageSlot::QrCode:
        return qtlogin::config::LoginMethod::QrCode;
    case LoginPageSlot::SmsCode:
        return qtlogin::config::LoginMethod::SafePhoneSms;
    }
    return qtlogin::config::LoginMethod::Password;
}

bool serverAllowsLoginSlot(const LoginWindowContext& context, LoginPageSlot slot)
{
    if (!hasUsableServerClientConfig(context) || context.serverClientConfig.loginMethodList.empty()) {
        return true;
    }
    return qtlogin::config::hasLoginMethod(context.serverClientConfig, methodForSlot(slot));
}

QString clientConfigUtf8(const std::string& text)
{
    return QString::fromUtf8(text.c_str(), static_cast<int>(text.size()));
}

QString accountPlaceholderText(const LoginWindowContext& context, const QString& fallback)
{
    if (hasUsableServerClientConfig(context) &&
        context.serverClientConfig.supportMobileAccountOnly &&
        !context.serverClientConfig.supportMobileAccountOnlyTip.empty()) {
        return clientConfigUtf8(context.serverClientConfig.supportMobileAccountOnlyTip);
    }
    return fallback;
}

QString passwordPlaceholderText(const LoginWindowContext& context)
{
    if (hasUsableServerClientConfig(context) &&
        context.serverClientConfig.forbiddenStaticPwd &&
        !context.serverClientConfig.forbiddenStaticPwdTip.empty()) {
        return clientConfigUtf8(context.serverClientConfig.forbiddenStaticPwdTip);
    }
    return QString();
}

QPixmap removeEmbeddedAlphaFromPixmap(const QPixmap& pixmap, int cutoff)
{
    if (pixmap.isNull() || !pixmap.hasAlphaChannel()) {
        return pixmap;
    }

    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    const int threshold = std::max(0, std::min(255, cutoff));
    for (int y = 0; y < image.height(); ++y) {
        auto* pixels = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = pixels[x];
            const int alpha = qAlpha(pixel);
            if (alpha >= threshold) {
                pixels[x] = qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), 255);
            } else {
                pixels[x] = qRgba(0, 0, 0, 0);
            }
        }
    }
    return QPixmap::fromImage(image);
}

}

LoginWindow::LoginWindow(const LoginWindowContext& context, QWidget* parent)
    : QWidget(parent), context_(context)
{
    skinRoot_ = QDir(QApplication::applicationDirPath()).filePath(QStringLiteral("skin"));
    background_ = QPixmap(skinPath(QStringLiteral("container/container_frame.png")));
    if (shouldRemoveEmbeddedWindowAlpha(context_.embedWindow, background_.hasAlphaChannel())) {
        background_ = removeEmbeddedAlphaFromPixmap(background_, embeddedWindowAlphaCutoff());
    }
    const double environmentScale = legacyDpiScaleFactorFromEnvironment();
    const int systemDpi = currentSystemDpiForLegacyScaling();
    legacyDpiScaleFactor_ = chooseLegacyDpiScaleFactor(context_.legacyDpiScaleFactor, environmentScale, systemDpi);
    if (legacyDpiScaleFactor_ <= 0.0) {
        legacyDpiScaleFactor_ = 1.0;
    }

    const LegacyRect windowRect = scaleLegacyRect({0, 0, 442, 404}, legacyDpiScaleFactor_);
    setFixedSize(windowRect.width, windowRect.height);
    if (!isUnitScale(legacyDpiScaleFactor_)) {
        QFont scaledFont = font();
        scaledFont.setPixelSize(scaleLegacyPixel(12, legacyDpiScaleFactor_));
        setFont(scaledFont);
    }
    {
        std::wostringstream out;
        out << L"legacy dpi scale=" << legacyDpiScaleFactor_
            << L" systemDpi=" << systemDpi
            << L" envScale=" << environmentScale
            << L" loginSize=" << windowRect.width << L"x" << windowRect.height;
        qtlogin::common::logLine(L"sdologin", out.str());
    }
    setWindowTitle(QString::fromStdWString(context_.appName.empty() ? L"盛趣游戏登录" : context_.appName));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_NativeWindow, context_.embedWindow);
    setAttribute(Qt::WA_TranslucentBackground, !context_.embedWindow);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, context_.embedWindow);
    setAutoFillBackground(false);
    if (!background_.isNull() && shouldMaskEmbeddedWindowBackground(context_.embedWindow, background_.hasAlphaChannel())) {
        const QPixmap maskSource = isUnitScale(legacyDpiScaleFactor_)
            ? background_
            : background_.scaled(size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);
        setMask(maskSource.mask());
    }
    passwordAuthenticator_ = std::make_unique<SdoBasePasswordAuthenticator>(authConfigFromContext(context_));
    if (passwordAuthenticator_) {
        const UserPrivacyConfigResult privacyResult = passwordAuthenticator_->fetchUserPrivacyConfig(
            context_.savedPrivacyPolicyVersion,
            context_.savedServiceAgreementVersion,
            3000);
        if (privacyResult.success) {
            context_.protocolReviewRequired = shouldRequireProtocolReviewAfterServerConfig(
                context_.savedPrivacyPolicyVersion,
                true,
                privacyResult.privacyPolicyVersion);
            context_.acceptedPrivacyPolicyVersion = acceptedPrivacyPolicyVersionForStorage(
                true,
                privacyResult.privacyPolicyVersion);
            context_.acceptedServiceAgreementVersion = privacyResult.serviceAgreementVersion;
            if (!privacyResult.privacyPolicyUrl.empty()) {
                context_.privacyPolicyUrl = privacyResult.privacyPolicyUrl;
            }
            if (!privacyResult.serviceAgreementUrl.empty()) {
                context_.serviceAgreementUrl = privacyResult.serviceAgreementUrl;
            }
            if (privacyResult.serviceAgreementVersion >= 0) {
                qtlogin::config::ConfigManager config;
                if (config.load(qtlogin::common::currentExecutableDirectory())) {
                    config.setIntegerValue(
                        qtlogin::config::ConfigFile::UserInfo,
                        L"ServicerAgreementVersion",
                        privacyResult.serviceAgreementVersion);
                }
            }
        } else {
            context_.protocolReviewRequired = shouldRequireProtocolReviewAfterServerConfig(
                context_.savedPrivacyPolicyVersion,
                false,
                -1);
            context_.acceptedPrivacyPolicyVersion = -1;
        }
        std::wostringstream out;
        out << L"user privacy config attempted=" << privacyResult.attempted
            << L" success=" << privacyResult.success
            << L" timedOut=" << privacyResult.timedOut
            << L" errorCode=" << privacyResult.errorCode
            << L" savedPrivacy=" << context_.savedPrivacyPolicyVersion
            << L" serverPrivacy=" << privacyResult.privacyPolicyVersion
            << L" showProtocol=" << context_.protocolReviewRequired;
        qtlogin::common::logLine(L"sdologin", out.str());
    }
    if (context_.fetchClientConfig && passwordAuthenticator_) {
        const StartupClientConfigResult configResult = passwordAuthenticator_->fetchStartupClientConfig(10000);
        if (configResult.success) {
            context_.serverClientConfig = configResult.config;
            context_.hasServerClientConfig = true;
        }
        std::wostringstream out;
        out << L"startup getSystemConfig attempted=" << configResult.attempted
            << L" success=" << configResult.success
            << L" timedOut=" << configResult.timedOut
            << L" errorCode=" << configResult.errorCode
            << L" rawLen=" << configResult.rawConfig.size();
        qtlogin::common::logLine(L"sdologin", out.str());
    }
    sharedAccountText_ = QString::fromStdWString(preferredAccountHistoryText(context_.accountHistory));
    buildUi();
    scaleLegacyWidgetTree(this, legacyDpiScaleFactor_);
    scaleLegacyFontTree(this, legacyDpiScaleFactor_);
    scaleLegacyStyleSheetTree(this, legacyDpiScaleFactor_);
    QTimer::singleShot(300, this, &LoginWindow::prewarmProtocolBrowser);
}

void LoginWindow::setCompletionHandler(std::function<void(const LoginWindowResult&)> handler)
{
    completionHandler_ = std::move(handler);
}

void LoginWindow::triggerAutoSuccess()
{
    completeSuccess(QStringLiteral("auto"));
}

void LoginWindow::applyEmbedding()
{
    if (!context_.embedWindow || !context_.ownerHwnd) {
        return;
    }

    HWND child = reinterpret_cast<HWND>(winId());
    HWND parent = reinterpret_cast<HWND>(context_.ownerHwnd);
    DWORD parentProcessId = 0;
    const DWORD parentThreadId = GetWindowThreadProcessId(parent, &parentProcessId);
    const DWORD childThreadId = GetCurrentThreadId();
    if (parentThreadId && parentThreadId != childThreadId) {
        AttachThreadInput(childThreadId, parentThreadId, TRUE);
    }

    LONG_PTR parentStyle = GetWindowLongPtrW(parent, GWL_STYLE);
    parentStyle |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    SetWindowLongPtrW(parent, GWL_STYLE, parentStyle);

    SetWindowLongPtrW(child, GWL_STYLE, makeEmbeddedChildStyle(GetWindowLongPtrW(child, GWL_STYLE)));
    SetWindowLongPtrW(child, GWL_EXSTYLE, makeEmbeddedChildExStyle(GetWindowLongPtrW(child, GWL_EXSTYLE)));
    SetParent(child, parent);

    SetWindowPos(child, HWND_TOP, context_.posX, context_.posY, width(), height(), SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    RECT windowRect{};
    RECT clientRect{};
    GetWindowRect(child, &windowRect);
    GetClientRect(child, &clientRect);
    qtlogin::common::logLine(
        L"sdologin",
        L"attached login child hwnd=" + hwndText(child) +
            L" parent=" + hwndText(parent) +
            L" parentThread=" + std::to_wstring(parentThreadId) +
            L" childThread=" + std::to_wstring(childThreadId) +
            L" logical=" + std::to_wstring(width()) + L"x" + std::to_wstring(height()) +
            L" dpr=" + std::to_wstring(devicePixelRatioF()) +
            L" windowRect=" + rectText(windowRect) +
            L" clientRect=" + rectText(clientRect));
}

QString LoginWindow::skinPath(const QString& relative) const
{
    return QDir(skinRoot_).filePath(relative);
}

void LoginWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, !context_.embedWindow);
    if (!background_.isNull()) {
        painter.drawPixmap(rect(), background_);
    } else {
        painter.fillRect(rect(), QColor(48, 30, 18));
        painter.setPen(QColor(255, 237, 220));
        painter.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 8, 8);
    }
}

void LoginWindow::mousePressEvent(QMouseEvent* event)
{
    if (shouldAllowLoginWindowDrag(context_.embedWindow) && event->button() == Qt::LeftButton) {
        dragOffset_ = globalMousePosition(event) - frameGeometry().topLeft();
    }
}

void LoginWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (shouldAllowLoginWindowDrag(context_.embedWindow) && (event->buttons() & Qt::LeftButton)) {
        move(globalMousePosition(event) - dragOffset_);
    }
}

void LoginWindow::closeEvent(QCloseEvent* event)
{
    if (!completed_) {
        completeCancel();
    }
    event->accept();
}

void LoginWindow::buildUi()
{
    auto* closeButton = new QPushButton(this);
    closeButton->setGeometry(407, 10, 22, 22);
    setButtonImages(closeButton,
        skinPath(QStringLiteral("container/btn_close_n.png")),
        skinPath(QStringLiteral("container/btn_close_o.png")),
        skinPath(QStringLiteral("container/btn_close_c.png")));
    connect(closeButton, &QPushButton::clicked, this, [this]() { close(); });

    auto* stack = new QStackedWidget(this);
    stack->setGeometry(0, 0, 442, 190);
    stack->setObjectName(QStringLiteral("loginStack"));

    auto* passwordPage = new QWidget(stack);
    auto* pushMessagePage = new QWidget(stack);
    auto* qrPage = new QWidget(stack);
    auto* smsPage = new QWidget(stack);
    buildPasswordPage(passwordPage);
    buildQuickLoginPage(pushMessagePage);
    CodeKeyLoginPanel* qrPanel = buildQrPage(qrPage);
    buildSmsPage(smsPage);
    stack->addWidget(passwordPage);
    stack->addWidget(pushMessagePage);
    stack->addWidget(qrPage);
    stack->addWidget(smsPage);

    QList<QPushButton*> tabs;
    const MainLoginTabSpec loginTabs = mainLoginTabSpec();
    const QList<LoginPageSlot> tabSlots = {
        loginTabs.passwordTab,
        loginTabs.oneClickTab,
        loginTabs.qrTab,
    };
    const struct TabSpec {
        int x;
        QString normal;
        QString hover;
        QString selected;
    } tabSpecs[] = {
        {49, QStringLiteral("container/tab_1_n.png"), QStringLiteral("container/tab_1_o.png"), QStringLiteral("container/tab_1_c.png")},
        {163, QStringLiteral("container/tab_2_n.png"), QStringLiteral("container/tab_2_o.png"), QStringLiteral("container/tab_2_c.png")},
        {277, QStringLiteral("container/tab_3_n.png"), QStringLiteral("container/tab_3_o.png"), QStringLiteral("container/tab_3_c.png")},
    };

    int firstVisibleTab = -1;
    for (int i = 0; i < 3; ++i) {
        auto* tab = new QPushButton(this);
        tab->setCheckable(true);
        tab->setGeometry(tabSpecs[i].x, 204, 108, 44);
        setButtonImages(tab, skinPath(tabSpecs[i].normal), skinPath(tabSpecs[i].hover), skinPath(tabSpecs[i].selected), skinPath(tabSpecs[i].selected));
        const bool allowedByServer = serverAllowsLoginSlot(context_, tabSlots[i]);
        tab->setVisible(allowedByServer);
        tab->setEnabled(allowedByServer);
        if (allowedByServer && firstVisibleTab < 0) {
            firstVisibleTab = i;
        }
        tabs.push_back(tab);
        connect(tab, &QPushButton::clicked, this, [this, stack, tabs, tabSlots, qrPanel, i]() mutable {
            stack->setCurrentIndex(loginPageIndex(tabSlots[i]));
            for (int tabIndex = 0; tabIndex < tabs.size(); ++tabIndex) {
                tabs[tabIndex]->setChecked(tabIndex == i);
            }
            if (qrPanel) {
                if (tabSlots[i] == LoginPageSlot::QrCode) {
                    qrPanel->activate();
                } else {
                    qrPanel->deactivate();
                }
            }
        });
    }
    if (firstVisibleTab < 0) {
        firstVisibleTab = 0;
        tabs[0]->setVisible(true);
        tabs[0]->setEnabled(true);
    }
    stack->setCurrentIndex(loginPageIndex(tabSlots[firstVisibleTab]));
    tabs[firstVisibleTab]->setChecked(true);
    if (qrPanel && tabSlots[firstVisibleTab] == LoginPageSlot::QrCode) {
        QTimer::singleShot(0, qrPanel, [qrPanel]() {
            qrPanel->activate();
        });
    }

    auto* saveCheck = new QCheckBox(this);
    saveCheck->setGeometry(50, 151, 18, 15);
    saveCheck->setStyleSheet(checkBoxIndicatorStyle(
        skinPath(QStringLiteral("common/kk.png")),
        skinPath(QStringLiteral("common/aa.png"))));

    auto* autoLogin = new QLabel(this);
    autoLogin->setGeometry(66, 151, 66, 17);
    setImageLabel(autoLogin, skinPath(QStringLiteral("container/autologin.png")));

    auto* registerLink = new QLabel(this);
    registerLink->setGeometry(134, 151, 80, 17);
    setImageLabel(registerLink, skinPath(QStringLiteral("container/register.png")));

    auto* findPassword = new QPushButton(this);
    findPassword->setGeometry(218, 151, 80, 17);
    findPassword->setCursor(Qt::PointingHandCursor);
    setButtonImages(findPassword, skinPath(QStringLiteral("container/findpassword.png")));
    connect(findPassword, &QPushButton::clicked, this, [this]() {
        if (!openForgotPasswordPage(context_)) {
            showHint(QStringLiteral("打开找回密码页面失败。"));
        }
    });

    auto* quickLogin = new QPushButton(this);
    quickLogin->setGeometry(296, 151, 92, 17);
    setButtonImages(quickLogin, skinPath(QStringLiteral("container/fastlogin.png")));
    quickLogin->setVisible(context_.quickLoginEnabled && serverAllowsLoginSlot(context_, loginTabs.quickShortcut));
    connect(quickLogin, &QPushButton::clicked, this, [this, stack, tabs, loginTabs]() mutable {
        stack->setCurrentIndex(loginPageIndex(loginTabs.quickShortcut));
        for (auto* tab : tabs) {
            tab->setChecked(false);
        }
    });

    auto* protocolCheck = new QCheckBox(this);
    protocolCheck->setObjectName(QStringLiteral("protocolCheck"));
    protocolCheck->setChecked(initialProtocolCheckboxChecked(context_.protocolReviewRequired));
    protocolCheck->setGeometry(47, 319, 18, 15);
    protocolCheck->setStyleSheet(saveCheck->styleSheet());
    connect(protocolCheck, &QCheckBox::toggled, this, [this, stack, qrPanel](bool checked) {
        const bool wasChecked = !checked;
        const bool shouldOpenProtocol = shouldOpenInitialPrivacyProtocol(
            context_.protocolReviewRequired,
            wasChecked,
            checked,
            initialPrivacyProtocolOpened_);
        if (shouldOpenProtocol) {
            initialPrivacyProtocolOpened_ = true;
            showProtocolDialog(
                QString::fromUtf8(u8"隐私政策"),
                QString::fromStdWString(context_.privacyPolicyUrl),
                context_.protocolAgreeDelaySeconds,
                true);
        }
        if (checked && qrPanel && stack->currentIndex() == loginPageIndex(LoginPageSlot::QrCode)) {
            qrPanel->activate();
        }
    });

    auto* protocolIntro = new QLabel(QStringLiteral("我已详细阅读并同意"), this);
    protocolIntro->setGeometry(64, 313, 108, 28);
    protocolIntro->setStyleSheet(QStringLiteral("QLabel{color:#ffeddc;font:12px 'Microsoft YaHei';background:transparent;}"));

    const QString protocolButtonStyle = QStringLiteral(
        "QPushButton{border:0;color:#ffeddc;background:transparent;text-align:left;font:12px 'Microsoft YaHei';}"
        "QPushButton:hover{text-decoration:underline;color:#ffffff;}");
    auto makeProtocolButton = [this, &protocolButtonStyle](const QString& text, const QRect& rect, const QString& title, const std::wstring& url) {
        auto* button = new QPushButton(text, this);
        button->setGeometry(rect);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(protocolButtonStyle);
        connect(button, &QPushButton::clicked, this, [this, title, url]() {
            showProtocolDialog(title, QString::fromStdWString(url));
        });
        return button;
    };
    makeProtocolButton(QStringLiteral("《隐私政策》"), QRect(169, 313, 74, 28), QStringLiteral("隐私政策"), context_.privacyPolicyUrl);
    makeProtocolButton(QStringLiteral("《用户协议》"), QRect(244, 313, 74, 28), QStringLiteral("用户协议"), context_.serviceAgreementUrl);
    makeProtocolButton(QStringLiteral("《补充规则》"), QRect(319, 313, 82, 28), QStringLiteral("补充规则"), context_.supplementaryRulesUrl);

    auto* appLabel = new QLabel(this);
    appLabel->setGeometry(49, 360, 246, 18);
    appLabel->setStyleSheet(QStringLiteral("QLabel{color:#d8b48a;font:12px 'Microsoft YaHei';background:transparent;}"));
    appLabel->setText(QStringLiteral("当前应用：%1").arg(QString::fromStdWString(context_.appName.empty() ? L"QtLoginRewrite" : context_.appName)));

    auto* dialogsButton = new QPushButton(QStringLiteral("旧窗口"), this);
    dialogsButton->setGeometry(308, 358, 72, 22);
    dialogsButton->setStyleSheet(QStringLiteral("QPushButton{border:1px solid rgba(255,237,220,120);color:#ffeddc;background:rgba(60,36,22,110);font:12px 'Microsoft YaHei';} QPushButton:hover{background:rgba(120,70,32,160);}"));
    connect(dialogsButton, &QPushButton::clicked, this, &LoginWindow::showLegacyDialogGallery);

    closeButton->raise();
}

void LoginWindow::buildPasswordPage(QWidget* page)
{
    auto* accountIcon = new QLabel(page);
    accountIcon->setGeometry(53, 53, 40, 22);
    setImageLabel(accountIcon, skinPath(QStringLiteral("login/account.png")));

    AccountHistoryCombo::Options accountOptions;
    accountOptions.skinRoot = skinRoot_;
    accountOptions.placeholder = accountPlaceholderText(context_, QStringLiteral("手机/邮箱/个性账号"));
    accountOptions.popupFontPixelSize = accountHistoryPopupFontPixelSize(legacyDpiScaleFactor_);
    accountOptions.accounts = context_.accountHistory;
    accountOptions.removeHandler = [this](const std::wstring& account) {
        removeHistoryAccount(account);
    };
    auto* accountCombo = new AccountHistoryCombo(std::move(accountOptions), page);
    QLineEdit* accountEdit = accountCombo->lineEdit();
    accountEdit->setObjectName(QStringLiteral("accountEdit"));
    accountCombo->setGeometry(96, 51, 177, 29);
    accountEdit->setGeometry(accountCombo->rect());
    accountEdit->setPlaceholderText(accountPlaceholderText(context_, QStringLiteral("手机/邮箱/个性账号")));
    accountEdit->setPlaceholderText(QStringLiteral("手机/邮箱/个性账号"));
    accountEdit->setStyleSheet(editStyle(skinPath(QStringLiteral("combobox/combo_n.png")), skinPath(QStringLiteral("combobox/combo_o.png"))));
    accountEdit->setPlaceholderText(accountPlaceholderText(context_, QStringLiteral("手机/邮箱/个性账号")));

    registerSharedAccountEdit(accountEdit);

    auto* passwordIcon = new QLabel(page);
    passwordIcon->setGeometry(53, 91, 40, 22);
    setImageLabel(passwordIcon, skinPath(QStringLiteral("login/password.png")));

    auto* passwordEdit = new QLineEdit(page);
    passwordEdit->setObjectName(QStringLiteral("passwordEdit"));
    passwordEdit->setGeometry(96, 88, 177, 28);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setMaxLength(30);
    passwordEdit->setPlaceholderText(passwordPlaceholderText(context_));
    passwordEdit->setStyleSheet(editStyle(skinPath(QStringLiteral("common/editbox_0.png")), skinPath(QStringLiteral("common/editbox_o.png"))));

    auto* keyboard = new QPushButton(page);
    keyboard->setGeometry(247, 95, 18, 14);
    setButtonImages(keyboard,
        skinPath(QStringLiteral("login/btn_keyboard_n.png")),
        skinPath(QStringLiteral("login/btn_keyboard_o.png")),
        skinPath(QStringLiteral("login/btn_keyboard_c.png")));
    disconnect(keyboard, nullptr, this, nullptr);
    connect(keyboard, &QPushButton::clicked, this, [this, passwordEdit]() { showKeyboardForPassword(passwordEdit, nullptr); });
    connect(keyboard, &QPushButton::clicked, this, [this]() { showHint(QStringLiteral("安全键盘将在后续阶段接入。")); });

    disconnect(keyboard, nullptr, this, nullptr);
    connect(keyboard, &QPushButton::clicked, this, [this, passwordEdit]() { showKeyboardForPassword(passwordEdit, nullptr); });

    auto* loginButton = new QPushButton(page);
    const SkinButtonSpec connectButton = sharedConnectButtonSpec();
    applyButtonSpec(loginButton, connectButton, [this](const std::string& relative) {
        return skinPath(QString::fromStdString(relative));
    });
    connect(loginButton, &QPushButton::clicked, this, [this, accountEdit, passwordEdit]() {
        auto* protocol = findChild<QCheckBox*>(QStringLiteral("protocolCheck"));
        if (protocol && !protocol->isChecked()) {
            showHint(QStringLiteral("请先阅读并同意协议。"));
            return;
        }
        if (accountEdit->text().trimmed().isEmpty() || passwordEdit->text().isEmpty()) {
            showHint(QStringLiteral("请输入账号和密码。"));
            return;
        }
        completeSuccess(QStringLiteral("password"));
    });
    disconnect(loginButton, nullptr, this, nullptr);
    connect(loginButton, &QPushButton::clicked, this, [this, accountEdit, passwordEdit, loginButton]() {
        submitPasswordLogin(accountEdit, passwordEdit, loginButton);
    });
}

void LoginWindow::buildSmsPage(QWidget* page)
{
    auto* phoneLabel = new QLabel(page);
    phoneLabel->setGeometry(58, 34, 52, 20);
    setImageLabel(phoneLabel, skinPath(QStringLiteral("common/label1.png")));

    auto* phoneEdit = new QLineEdit(page);
    phoneEdit->setGeometry(120, 29, 204, 32);
    phoneEdit->setPlaceholderText(QStringLiteral("绑定手机的账号/手机账号"));
    phoneEdit->setStyleSheet(editStyle(skinPath(QStringLiteral("combobox/combo_n.png")), skinPath(QStringLiteral("combobox/combo_o.png"))));

    auto* codeLabel = new QLabel(page);
    codeLabel->setGeometry(58, 73, 52, 20);
    setImageLabel(codeLabel, skinPath(QStringLiteral("common/label3.png")));

    auto* codeEdit = new QLineEdit(page);
    codeEdit->setGeometry(120, 68, 204, 32);
    codeEdit->setPlaceholderText(QStringLiteral("验证码"));
    codeEdit->setMaxLength(6);
    codeEdit->setStyleSheet(editStyle(skinPath(QStringLiteral("common/editbox_0.png")), skinPath(QStringLiteral("common/editbox_o.png"))));

    auto* getCode = new QPushButton(QStringLiteral("免费获取验证码"), page);
    getCode->setGeometry(197, 68, 127, 32);
    setButtonImages(getCode,
        skinPath(QStringLiteral("cellphone_register/btn_sernum_n.png")),
        skinPath(QStringLiteral("cellphone_register/btn_sernum_o.png")),
        skinPath(QStringLiteral("cellphone_register/btn_sernum_c.png")));
    connect(getCode, &QPushButton::clicked, this, [this]() { showHint(QStringLiteral("短信发送接口将在服务接入阶段实现。")); });

    auto* okButton = new QPushButton(page);
    okButton->setGeometry(117, 124, 211, 46);
    setButtonImages(okButton,
        skinPath(QStringLiteral("common/btn_login_n.png")),
        skinPath(QStringLiteral("common/btn_login_o.png")),
        skinPath(QStringLiteral("common/btn_login_c.png")));
    connect(okButton, &QPushButton::clicked, this, [this, phoneEdit, codeEdit]() {
        if (phoneEdit->text().trimmed().isEmpty() || codeEdit->text().trimmed().isEmpty()) {
            showHint(QStringLiteral("请输入手机号/账号和验证码。"));
            return;
        }
        completeSuccess(QStringLiteral("sms"));
    });
}

CodeKeyLoginPanel* LoginWindow::buildQrPage(QWidget* page)
{
    CodeKeyLoginPanelOptions options;
    options.skinRoot = skinRoot_;
    options.authConfig = authConfigFromContext(context_);
    options.daoyuHomeUrl = context_.daoyuHomeUrl;
    options.refreshClickIntervalMs = context_.codeKeyRefreshClickIntervalMs;
    options.protocolAccepted = [this]() {
        auto* protocol = findChild<QCheckBox*>(QStringLiteral("protocolCheck"));
        return protocol && protocol->isChecked();
    };
    options.hintHandler = [this](const QString& text) {
        showHint(text);
    };
    options.completionHandler = [this](const CodeKeyLoginResult& result) {
        completeCodeKeyLogin(result);
    };

    auto* panel = new CodeKeyLoginPanel(std::move(options), page);
    panel->setGeometry(0, 0, 442, 190);
    return panel;
}

void LoginWindow::buildQuickLoginPage(QWidget* page)
{
    PushMessageLoginPanelOptions options;
    options.authConfig = authConfigFromContext(context_);
    options.skinRoot = skinRoot_;
    options.accountPlaceholder = accountPlaceholderText(context_, QString::fromUtf8(u8"手机/邮箱/个性账号"));
    options.daoyuHomeUrl = context_.daoyuHomeUrl;
    options.accountPopupFontPixelSize = accountHistoryPopupFontPixelSize(legacyDpiScaleFactor_);
    options.accountHistory = context_.accountHistory;
    options.removeAccountHandler = [this](const std::wstring& account) {
        removeHistoryAccount(account);
    };
    options.successfulAccountHandler = [this](const QString& account) {
        lastSubmittedPushAccount_ = account.trimmed();
    };
    options.supportMobileAccountOnly =
        context_.hasServerClientConfig && context_.serverClientConfig.supportMobileAccountOnly;
    options.protocolAccepted = [this]() {
        auto* protocol = findChild<QCheckBox*>(QStringLiteral("protocolCheck"));
        return !protocol || protocol->isChecked();
    };
    options.hintHandler = [this](const QString& text) {
        showHint(text);
    };
    options.completionHandler = [this](const PushMessageLoginResult& result) {
        completePushMessageLogin(result);
    };

    auto* panel = new PushMessageLoginPanel(std::move(options), page);
    panel->setGeometry(0, 0, 442, 190);
    registerSharedAccountEdit(panel->accountEdit());
    return;

#if 0
    const QuickLoginSkinSpec skin = quickLoginSkinSpec();
    const auto skinAsset = [this](const std::string& relative) {
        return skinPath(QString::fromStdString(relative));
    };

    auto* download = new QPushButton(QStringLiteral("需安装叨鱼"), page);
    download->setGeometry(96, 22, 177, 22);
    download->setCursor(Qt::PointingHandCursor);
    download->setStyleSheet(QStringLiteral(
        "QPushButton{border:0;color:#ffeddc;background:transparent;text-align:center;font:bold 14px 'Microsoft YaHei';}"
        "QPushButton:hover{text-decoration:underline;color:#ffffff;}"));
    connect(download, &QPushButton::clicked, this, [this]() {
        if (!openDaoyuHomePage(context_)) {
            showHint(QStringLiteral("打开叨鱼官网失败。"));
        }
    });

    auto* accountIcon = new QLabel(page);
    accountIcon->setGeometry(53, 53, 40, 22);
    setImageLabel(accountIcon, skinPath(QStringLiteral("login/account.png")));

    auto* accountEdit = new QLineEdit(page);
    accountEdit->setGeometry(96, 51, 177, 29);
    accountEdit->setPlaceholderText(QStringLiteral("手机/邮箱/个性账号"));
    accountEdit->setStyleSheet(editStyle(skinPath(QStringLiteral("combobox/combo_n.png")), skinPath(QStringLiteral("combobox/combo_o.png"))));

    accountEdit->setPlaceholderText(accountPlaceholderText(context_, QStringLiteral("手机/邮箱/个性账号")));

    auto* statusText = new QLabel(page);
    statusText->setObjectName(QStringLiteral("pushMessageStatusHintText"));
    statusText->setGeometry(96, 88, 220, 36);
    statusText->setStyleSheet(QStringLiteral("QLabel{color:#ffeddc;background:transparent;font:16px 'Microsoft YaHei';}"));
    statusText->setVisible(false);

    auto* loginButton = new QPushButton(page);
    loginButton->setObjectName(QStringLiteral("pushMessageLoginButton"));
    applyButtonSpec(loginButton, skin.actionButton, skinAsset);
    loginButton->setText(QString());

    auto* retryTimer = new QTimer(loginButton);
    retryTimer->setInterval(1000);
    connect(retryTimer, &QTimer::timeout, this, [loginButton, retryTimer, skin, skinAsset]() {
        int remaining = loginButton->property("retrySeconds").toInt() - 1;
        loginButton->setProperty("retrySeconds", remaining);
        if (remaining <= 0) {
            retryTimer->stop();
            loginButton->setEnabled(true);
            loginButton->setText(QString());
            applyButtonSpec(loginButton, skin.actionButton, skinAsset);
            return;
        }
        loginButton->setText(QStringLiteral("%1秒后重试").arg(remaining));
    });

    connect(loginButton, &QPushButton::clicked, this, [this, accountEdit, statusText, loginButton, retryTimer, skin, skinAsset]() {
        if (accountEdit->text().trimmed().isEmpty()) {
            showHint(QStringLiteral("请输入账号后再发起一键登录。"));
            return;
        }
        statusText->setText(QStringLiteral("登录确认码:  123456"));
        statusText->setVisible(true);
        applyButtonSpec(loginButton, skin.retryButton, skinAsset);
        loginButton->setEnabled(false);
        loginButton->setProperty("retrySeconds", skin.retryCountdownSeconds);
        loginButton->setText(QStringLiteral("%1秒后重试").arg(skin.retryCountdownSeconds));
        retryTimer->start();
        QTimer::singleShot(2500, this, [this]() {
            completeSuccess(QStringLiteral("push"));
        });
    });
#endif
}

void LoginWindow::submitPasswordLogin(QLineEdit* accountEdit, QLineEdit* passwordEdit, QPushButton* loginButton)
{
    auto* protocol = findChild<QCheckBox*>(QStringLiteral("protocolCheck"));
    PasswordLoginRequest request;
    request.account = accountEdit ? accountEdit->text().toStdWString() : std::wstring();
    request.password = passwordEdit ? passwordEdit->text().toStdWString() : std::wstring();
    request.protocolAccepted = !protocol || protocol->isChecked();
    request.inputUserType = 0;
    request.keepLogin = false;

    const PasswordLoginValidationResult validation = validatePasswordLoginRequest(request);
    if (validation.code != PasswordLoginValidationCode::Ok) {
        showHint(QString::fromStdWString(validation.message));
        return;
    }
    lastSubmittedPasswordAccount_ = QString::fromStdWString(validation.trimmedAccount);

    if (!passwordAuthenticator_) {
        showHint(QStringLiteral("密码认证模块未初始化。"));
        return;
    }

    if (loginButton) {
        loginButton->setEnabled(false);
    }
    if (keyboardDialog_) {
        keyboardDialog_->hide();
    }

    QPointer<LoginWindow> windowGuard(this);
    QPointer<QLineEdit> passwordGuard(passwordEdit);
    QPointer<QPushButton> buttonGuard(loginButton);
    passwordAuthenticator_->login(request, [windowGuard, passwordGuard, buttonGuard](const PasswordAuthResult& result) {
        if (!windowGuard) {
            return;
        }
        QMetaObject::invokeMethod(windowGuard, [windowGuard, passwordGuard, buttonGuard, result]() {
            if (!windowGuard) {
                return;
            }
            if (buttonGuard) {
                buttonGuard->setEnabled(true);
            }
            if (passwordGuard) {
                passwordGuard->clear();
            }
            if (result.success) {
                windowGuard->completePasswordLogin(result);
            } else {
                windowGuard->showHint(QString::fromStdWString(result.errorMessage));
            }
        }, Qt::QueuedConnection);
    });
}

void LoginWindow::showKeyboardForPassword(QLineEdit* passwordEdit, QPushButton* loginButton)
{
    if (!keyboardDialog_) {
        auto* keyboard = new VirtualKeyboardDialog(skinRoot_, legacyDpiScaleFactor_, this);
        keyboardDialog_ = keyboard;
        keyboard->setCharacterHandler([passwordEdit](QChar ch) {
            if (passwordEdit && !ch.isNull()) {
                passwordEdit->insert(QString(ch));
            }
        });
        keyboard->setBackspaceHandler([passwordEdit]() {
            if (passwordEdit) {
                passwordEdit->backspace();
            }
        });
        keyboard->setEnterHandler([this, passwordEdit, loginButton]() {
            auto* accountEdit = findChild<QLineEdit*>(QStringLiteral("accountEdit"));
            submitPasswordLogin(accountEdit, passwordEdit, loginButton);
        });
    }

    const QPoint topLeft = mapToGlobal(QPoint(0, scaleLegacyPixel(-212, legacyDpiScaleFactor_)));
    keyboardDialog_->move(topLeft);
    keyboardDialog_->show();
    keyboardDialog_->raise();
    keyboardDialog_->activateWindow();
}

void LoginWindow::completePasswordLogin(const PasswordAuthResult& authResult)
{
    if (completed_) {
        return;
    }
    completed_ = true;

    LoginWindowResult result;
    result.success = true;
    result.sessionId = authResult.sessionId;
    result.sndaid = authResult.sndaId;
    result.identityState = "1";
    result.ticket = authResult.ticket;
    recordSuccessfulAccount(
        lastSubmittedPasswordAccount_,
        0,
        shouldRefreshAccountHistoryUiAfterRecord(true));
    if (completionHandler_) {
        completionHandler_(result);
    }
    close();
}

void LoginWindow::completeCodeKeyLogin(const CodeKeyLoginResult& authResult)
{
    if (completed_) {
        return;
    }
    completed_ = true;

    LoginWindowResult result;
    result.success = true;
    result.sessionId = authResult.sessionId;
    result.sndaid = authResult.sndaId;
    result.identityState = "1";
    result.ticket = authResult.ticket;
    recordSuccessfulAccount(
        QString::fromStdString(authResult.inputUserId),
        0,
        shouldRefreshAccountHistoryUiAfterRecord(true));
    if (completionHandler_) {
        completionHandler_(result);
    }
    close();
}

void LoginWindow::completePushMessageLogin(const PushMessageLoginResult& authResult)
{
    if (completed_) {
        return;
    }
    completed_ = true;

    LoginWindowResult result;
    result.success = true;
    result.sessionId = authResult.sessionId;
    result.sndaid = authResult.sndaId;
    result.identityState = "1";
    result.ticket = authResult.ticket;
    recordSuccessfulAccount(
        lastSubmittedPushAccount_.isEmpty() ? QString::fromStdString(authResult.inputUserId) : lastSubmittedPushAccount_,
        0,
        shouldRefreshAccountHistoryUiAfterRecord(true));
    if (completionHandler_) {
        completionHandler_(result);
    }
    close();
}

void LoginWindow::completeSuccess(const QString& source)
{
    if (completed_) {
        return;
    }
    completed_ = true;

    LoginWindowResult result;
    result.success = true;
    result.sessionId = "qt-ui-session-" + source.toStdString();
    result.sndaid = "qt-ui-sndaid";
    result.identityState = "1";
    result.ticket = "qt-ui-ticket-" + source.toStdString();
    if (completionHandler_) {
        completionHandler_(result);
    }
    close();
}

void LoginWindow::completeCancel()
{
    if (completed_) {
        return;
    }
    completed_ = true;
    LoginWindowResult result;
    result.success = false;
    if (completionHandler_) {
        completionHandler_(result);
    }
}

void LoginWindow::showLegacyDialogGallery()
{
    const auto scalePx = [this](int value) {
        return scaleLegacyPixel(value, legacyDpiScaleFactor_);
    };

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("旧 DuiLib 窗口迁移清单"));
    dialog.setFixedSize(scalePx(720), scalePx(520));
    dialog.setModal(true);
    dialog.setStyleSheet(scaleStyleSheetPixels(QStringLiteral(
        "QDialog{background:#3c2418;color:#ffeddc;}"
        "QListWidget{background:#fff9ef;color:#3d2515;border:0;font:13px 'Microsoft YaHei';}"
        "QTextBrowser{background:#fff9ef;color:#3d2515;border:0;font:13px 'Microsoft YaHei';}"
        "QPushButton{background:#a65f24;color:white;border:0;font:14px 'Microsoft YaHei';padding:8px 18px;}"
        "QPushButton:hover{background:#bd7130;}"), legacyDpiScaleFactor_));

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(scalePx(18), scalePx(18), scalePx(18), scalePx(18));
    layout->setSpacing(scalePx(8));

    auto* body = new QHBoxLayout();
    body->setSpacing(scalePx(8));
    auto* list = new QListWidget(&dialog);
    auto* detail = new QTextBrowser(&dialog);
    body->addWidget(list, 2);
    body->addWidget(detail, 3);
    layout->addLayout(body);

    const auto& dialogs = qtlogin::ui_model::legacyDialogs();
    for (const auto& item : dialogs) {
        auto* row = new QListWidgetItem(QStringLiteral("%1  [%2]").arg(QString::fromWCharArray(item.title), QString::fromWCharArray(item.category)));
        row->setData(Qt::UserRole, QString::fromWCharArray(item.stableName));
        list->addItem(row);
    }

    auto renderDetail = [detail](const qtlogin::ui_model::LegacyDialogDescriptor& item) {
        detail->setHtml(QStringLiteral(
            "<h2>%1</h2>"
            "<p><b>旧类：</b>%2</p>"
            "<p><b>旧皮肤：</b><code>%3</code></p>"
            "<p><b>分类：</b>%4</p>"
            "<p><b>Qt 迁移状态：</b>%5</p>")
            .arg(QString::fromWCharArray(item.title),
                 QString::fromWCharArray(item.oldClassName),
                 QString::fromWCharArray(item.oldSkinFile),
                 QString::fromWCharArray(item.category),
                 QString::fromWCharArray(item.rewriteStatus)));
    };

    connect(list, &QListWidget::currentRowChanged, &dialog, [renderDetail, &dialogs](int row) {
        if (row >= 0 && row < static_cast<int>(dialogs.size())) {
            renderDetail(dialogs[static_cast<size_t>(row)]);
        }
    });
    if (!dialogs.empty()) {
        list->setCurrentRow(0);
    }

    auto* buttons = new QHBoxLayout();
    buttons->addStretch();
    auto* open = new QPushButton(QStringLiteral("打开骨架窗口"), &dialog);
    auto* close = new QPushButton(QStringLiteral("关闭"), &dialog);
    buttons->addWidget(open);
    buttons->addWidget(close);
    layout->addLayout(buttons);

    connect(open, &QPushButton::clicked, &dialog, [this, list, &dialogs, scalePx]() {
        const int row = list->currentRow();
        if (row < 0 || row >= static_cast<int>(dialogs.size())) {
            return;
        }
        const auto& item = dialogs[static_cast<size_t>(row)];
        QDialog skeleton(this);
        skeleton.setWindowTitle(QString::fromWCharArray(item.title));
        skeleton.setFixedSize(scalePx(460), scalePx(260));
        skeleton.setModal(true);
        skeleton.setStyleSheet(scaleStyleSheetPixels(QStringLiteral("QDialog{background:#3c2418;} QLabel{color:#ffeddc;font:13px 'Microsoft YaHei';} QPushButton{background:#a65f24;color:white;border:0;font:14px 'Microsoft YaHei';padding:8px 18px;}"), legacyDpiScaleFactor_));
        auto* skeletonLayout = new QVBoxLayout(&skeleton);
        skeletonLayout->setContentsMargins(scalePx(24), scalePx(24), scalePx(24), scalePx(18));
        skeletonLayout->setSpacing(scalePx(10));
        auto* text = new QLabel(QStringLiteral("%1\n\n旧类：%2\n旧皮肤：%3\n\n当前为 Qt 骨架窗口，后续按旧业务流程接入真实输入、校验和回调。")
            .arg(QString::fromWCharArray(item.title),
                 QString::fromWCharArray(item.oldClassName),
                 QString::fromWCharArray(item.oldSkinFile)), &skeleton);
        text->setWordWrap(true);
        skeletonLayout->addWidget(text);
        auto* ok = new QPushButton(QStringLiteral("确定"), &skeleton);
        skeletonLayout->addWidget(ok, 0, Qt::AlignRight);
        connect(ok, &QPushButton::clicked, &skeleton, &QDialog::accept);
        skeleton.exec();
    });
    connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

void LoginWindow::showProtocolDialog(
    const QString& title,
    const QString& url,
    int agreeDelaySeconds,
    bool recordInitialAcceptance)
{
    if (protocolDialogActive_) {
        return;
    }
    qtlogin::common::logLine(
        L"sdologin",
        L"protocol click title=" + title.toStdWString() +
            L" urlLen=" + std::to_wstring(url.size()) +
            L" agreeDelay=" + std::to_wstring(agreeDelaySeconds) +
            L" recordAcceptance=" + (recordInitialAcceptance ? std::wstring(L"1") : std::wstring(L"0")));
    showHint(QStringLiteral("正在打开协议页面..."));
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 30);
    ProtocolBrowserLaunchOptions options;
    options.title = title.toStdWString();
    options.url = url.toStdWString();
    options.ownerHwnd = reinterpret_cast<void*>(static_cast<uintptr_t>(winId()));
    options.legacyDpiScaleFactor = legacyDpiScaleFactor_;
    options.agreeDelaySeconds = agreeDelaySeconds;

    const std::wstring exePath = protocolBrowserExecutablePath(qtlogin::common::currentExecutableDirectory());
    QStringList arguments;
    for (const std::wstring& arg : buildProtocolBrowserArguments(options)) {
        arguments.push_back(QString::fromStdWString(arg));
    }

    auto* process = new QProcess(this);
    process->setProgram(QString::fromStdWString(exePath));
    process->setArguments(arguments);
    process->setWorkingDirectory(QString::fromStdWString(qtlogin::common::currentExecutableDirectory()));
    setProtocolDialogActive(true);
    connect(process, &QProcess::errorOccurred, this, [this, process, recordInitialAcceptance](QProcess::ProcessError error) {
        qtlogin::common::logLine(L"sdologin", L"protocol browser process error=" + std::to_wstring(static_cast<int>(error)));
        setProtocolDialogActive(false);
        if (recordInitialAcceptance) {
            initialPrivacyProtocolOpened_ = false;
            if (auto* protocol = findChild<QCheckBox*>(QStringLiteral("protocolCheck"))) {
                const QSignalBlocker blocker(protocol);
                protocol->setChecked(false);
            }
        }
        showHint(QStringLiteral("Protocol browser launch failed."));
        process->deleteLater();
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
        [this, process, recordInitialAcceptance](int exitCode, QProcess::ExitStatus exitStatus) {
            qtlogin::common::logLine(
                L"sdologin",
                L"protocol browser finished exitCode=" + std::to_wstring(exitCode) +
                    L" exitStatus=" + std::to_wstring(static_cast<int>(exitStatus)));
            setProtocolDialogActive(false);
            const bool accepted = exitStatus == QProcess::NormalExit &&
                shouldPersistInitialProtocolAcceptance(
                    recordInitialAcceptance,
                    exitCode,
                    context_.acceptedPrivacyPolicyVersion);
            if (accepted) {
                if (persistInitialProtocolAcceptance()) {
                    showHint(QStringLiteral("已记录协议同意状态。"));
                } else {
                    showHint(QStringLiteral("协议同意状态写入失败。"));
                }
            } else if (recordInitialAcceptance) {
                initialPrivacyProtocolOpened_ = false;
                if (auto* protocol = findChild<QCheckBox*>(QStringLiteral("protocolCheck"))) {
                    const QSignalBlocker blocker(protocol);
                    protocol->setChecked(false);
                }
            }
            process->deleteLater();
        });
    process->start();
}

void LoginWindow::setProtocolDialogActive(bool active)
{
    protocolDialogActive_ = active;
    auto* blocker = findChild<QWidget*>(QStringLiteral("protocolModalBlocker"));
    if (active) {
        if (!blocker) {
            blocker = new QWidget(this);
            blocker->setObjectName(QStringLiteral("protocolModalBlocker"));
            blocker->setAttribute(Qt::WA_StyledBackground, true);
            blocker->setStyleSheet(QStringLiteral("background:transparent;"));
            blocker->setFocusPolicy(Qt::StrongFocus);
        }
        blocker->setGeometry(rect());
        blocker->show();
        blocker->raise();
        blocker->setFocus();
        return;
    }
    if (blocker) {
        blocker->hide();
    }
}

bool LoginWindow::persistInitialProtocolAcceptance()
{
    qtlogin::config::ConfigManager config;
    if (!config.load(qtlogin::common::currentExecutableDirectory())) {
        qtlogin::common::logLine(L"sdologin", L"persist protocol acceptance failed: config load failed");
        return false;
    }
    if (!config.setIntegerValue(
            qtlogin::config::ConfigFile::UserInfo,
            L"PrivacyPolicyVersion",
            context_.acceptedPrivacyPolicyVersion)) {
        qtlogin::common::logLine(L"sdologin", L"persist protocol acceptance failed: PrivacyPolicyVersion write failed");
        return false;
    }
    context_.savedPrivacyPolicyVersion = context_.acceptedPrivacyPolicyVersion;
    context_.protocolReviewRequired = false;
    qtlogin::common::logLine(
        L"sdologin",
        L"persist protocol acceptance version=" + std::to_wstring(context_.acceptedPrivacyPolicyVersion));
    return true;
}

void LoginWindow::recordSuccessfulAccount(const QString& account, int type, bool refreshVisibleUi)
{
    const QString normalized = account.trimmed();
    if (normalized.isEmpty()) {
        return;
    }

    qtlogin::config::ConfigManager config;
    if (!config.load(qtlogin::common::currentExecutableDirectory())) {
        qtlogin::common::logLine(L"sdologin", L"record account failed: config load failed");
        return;
    }
    const int maxCount = config.maxAccountCount(context_.accountHistoryMaxCount);
    if (!config.recordUserAccount(normalized.toStdWString(), type, maxCount)) {
        qtlogin::common::logLine(L"sdologin", L"record account failed: write failed");
        return;
    }
    context_.accountHistoryMaxCount = maxCount;
    context_.accountHistory = config.userAccounts(maxCount);
    if (!refreshVisibleUi) {
        qtlogin::common::logLine(L"sdologin", L"record account config-only during login completion");
        return;
    }
    for (AccountHistoryCombo* combo : findChildren<AccountHistoryCombo*>()) {
        combo->setAccounts(context_.accountHistory);
    }
    updateSharedAccountText(nullptr, normalized);
}

void LoginWindow::removeHistoryAccount(const std::wstring& account)
{
    qtlogin::config::ConfigManager config;
    if (!config.load(qtlogin::common::currentExecutableDirectory())) {
        qtlogin::common::logLine(L"sdologin", L"remove account failed: config load failed");
        return;
    }
    config.removeUserAccount(account);
    context_.accountHistory = config.userAccounts(config.maxAccountCount(context_.accountHistoryMaxCount));
    for (AccountHistoryCombo* combo : findChildren<AccountHistoryCombo*>()) {
        combo->setAccounts(context_.accountHistory);
    }
    if (sharedAccountText_.compare(QString::fromStdWString(account), Qt::CaseInsensitive) == 0) {
        updateSharedAccountText(nullptr, QString::fromStdWString(preferredAccountHistoryText(context_.accountHistory)));
    }
}

void LoginWindow::registerSharedAccountEdit(QLineEdit* edit)
{
    if (!edit) {
        return;
    }
    const auto existing = std::find_if(sharedAccountEdits_.begin(), sharedAccountEdits_.end(), [edit](const QPointer<QLineEdit>& item) {
        return item.data() == edit;
    });
    if (existing == sharedAccountEdits_.end()) {
        sharedAccountEdits_.append(QPointer<QLineEdit>(edit));
    }
    if (!sharedAccountText_.isEmpty()) {
        const QSignalBlocker blocker(edit);
        edit->setText(sharedAccountText_);
    }
    connect(edit, &QLineEdit::textChanged, this, [this, edit](const QString& text) {
        updateSharedAccountText(edit, text);
    });
}

void LoginWindow::updateSharedAccountText(QLineEdit* source, const QString& text)
{
    const QString normalized = text.trimmed();
    if (sharedAccountText_ == normalized) {
        return;
    }
    sharedAccountText_ = normalized;
    for (int i = sharedAccountEdits_.size() - 1; i >= 0; --i) {
        if (sharedAccountEdits_.at(i).isNull()) {
            sharedAccountEdits_.removeAt(i);
        }
    }
    for (const QPointer<QLineEdit>& editPointer : sharedAccountEdits_) {
        QLineEdit* edit = editPointer.data();
        if (!edit || edit == source) {
            continue;
        }
        const QSignalBlocker blocker(edit);
        edit->setText(sharedAccountText_);
    }
}

void LoginWindow::prewarmProtocolBrowser()
{
    ProtocolBrowserLaunchOptions options;
    options.title = L"prewarm";
    options.url = L"about:blank";
    options.prewarmOnly = true;
    qtlogin::common::logLine(L"sdologin", L"protocol browser prewarm begin");
    launchProtocolBrowser(options);
}

void LoginWindow::showHint(const QString& text)
{
    auto* hint = findChild<QLabel*>(QStringLiteral("hintLabel"));
    if (!hint) {
        hint = new QLabel(this);
        hint->setObjectName(QStringLiteral("hintLabel"));
        hint->setAlignment(Qt::AlignCenter);
    }
    hint->setGeometry(scaleQtRect(QRect(95, 118, 300, 26), legacyDpiScaleFactor_));
    hint->setStyleSheet(scaleStyleSheetPixels(
        QStringLiteral("QLabel{background:rgba(55,32,18,210);color:#ffeddc;font:13px 'Microsoft YaHei';border-radius:3px;padding-left:6px;padding-right:6px;}"),
        legacyDpiScaleFactor_));
    hint->setText(text);
    hint->show();
    hint->raise();
    QTimer::singleShot(2200, hint, &QLabel::hide);
}

}
