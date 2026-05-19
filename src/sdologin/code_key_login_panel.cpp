#include "code_key_login_panel.h"

#include <QDesktopServices>
#include <QDir>
#include <QHideEvent>
#include <QLabel>
#include <QMetaObject>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QShowEvent>
#include <QUrl>

#include <utility>

#include "logger.h"

namespace qtlogin::sdologin {
namespace {

QString imageUrl(const QString& path)
{
    return QStringLiteral("url(\"%1\")").arg(QDir::fromNativeSeparators(path));
}

void setImageLabel(QLabel* label, const QString& path)
{
    QPixmap pixmap(path);
    if (!pixmap.isNull()) {
        label->setPixmap(pixmap);
        label->setScaledContents(true);
    }
}

void setButtonImages(QPushButton* button, const QString& normal, const QString& hover = QString(), const QString& pressed = QString())
{
    QString style = QStringLiteral("QPushButton{border:0;border-image:%1;background:transparent;color:#ffeddc;font:12px 'Microsoft YaHei';}").arg(imageUrl(normal));
    if (!hover.isEmpty()) {
        style += QStringLiteral("QPushButton:hover{border-image:%1;}").arg(imageUrl(hover));
    }
    if (!pressed.isEmpty()) {
        style += QStringLiteral("QPushButton:pressed{border-image:%1;}").arg(imageUrl(pressed));
    }
    button->setStyleSheet(style);
}

QString configuredDaoyuUrl(const std::wstring& value)
{
    if (value.empty()) {
        return QStringLiteral("https://www.daoyu8.com/#/");
    }
    return QString::fromWCharArray(value.c_str(), static_cast<int>(value.size()));
}

}

CodeKeyLoginPanel::CodeKeyLoginPanel(CodeKeyLoginPanelOptions options, QWidget* parent)
    : QWidget(parent)
    , options_(std::move(options))
    , authenticator_(std::make_unique<SdoBaseCodeKeyAuthenticator>(options_.authConfig))
    , pollTimer_(this)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("background:transparent;"));
    // LoginWindow applies legacy DPI scaling after building the widget tree.
    // Keep the panel resizable so scaled QR controls are not clipped by 442x190.
    resize(442, 190);
    pollTimer_.setSingleShot(true);
    pollTimer_.setInterval(1000);
    connect(&pollTimer_, &QTimer::timeout, this, [this]() {
        pollOnce();
    });
    refreshClickClock_.start();
    buildUi();
    setState(CodeKeyLoginState::Idle);
}

CodeKeyLoginPanel::~CodeKeyLoginPanel()
{
    deactivate();
}

void CodeKeyLoginPanel::activate()
{
    active_ = true;
    if (!isProtocolAccepted()) {
        setState(CodeKeyLoginState::Idle);
        showHint(QString::fromUtf8(u8"请先勾选协议后再使用扫码登录。"));
        return;
    }
    if (state_ == CodeKeyLoginState::Idle || state_ == CodeKeyLoginState::Expired || state_ == CodeKeyLoginState::Failed) {
        startNewFlow();
        return;
    }
    schedulePoll();
}

void CodeKeyLoginPanel::deactivate()
{
    active_ = false;
    pollTimer_.stop();
    requestInFlight_ = false;
    pollInFlight_ = false;
    if (authenticator_) {
        authenticator_->cancel();
    }
}

void CodeKeyLoginPanel::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    activate();
}

void CodeKeyLoginPanel::hideEvent(QHideEvent* event)
{
    deactivate();
    QWidget::hideEvent(event);
}

QString CodeKeyLoginPanel::skinPath(const QString& relative) const
{
    return QDir(options_.skinRoot).filePath(relative);
}

void CodeKeyLoginPanel::buildUi()
{
    auto* bg = new QLabel(this);
    bg->setGeometry(75, 22, 274, 121);
    setImageLabel(bg, skinPath(QStringLiteral("code_key/QR_bg.png")));

    refreshTopButton_ = new QPushButton(this);
    refreshTopButton_->setGeometry(203, 30, 92, 18);
    setButtonImages(refreshTopButton_,
        skinPath(QStringLiteral("code_key/btn_er_n.png")),
        skinPath(QStringLiteral("code_key/btn_er_o.png")),
        skinPath(QStringLiteral("code_key/btn_er_c.png")));
    connect(refreshTopButton_, &QPushButton::clicked, this, [this]() {
        startNewFlowFromRefreshClick();
    });

    auto* appButton = new QPushButton(this);
    appButton->setGeometry(204, 96, 70, 14);
    appButton->setCursor(Qt::PointingHandCursor);
    setButtonImages(appButton,
        skinPath(QStringLiteral("code_key/QR_btn_1.png")),
        skinPath(QStringLiteral("code_key/QR_btn_1_h.png")),
        skinPath(QStringLiteral("code_key/QR_btn_1.png")));
    connect(appButton, &QPushButton::clicked, this, [this]() {
        if (!openDaoyuHomePage()) {
            showHint(QString::fromUtf8(u8"打开叨鱼官网失败。"));
        }
    });

    auto* helpButton = new QPushButton(this);
    helpButton->setGeometry(205, 116, 140, 14);
    helpButton->setCursor(Qt::PointingHandCursor);
    setButtonImages(helpButton,
        skinPath(QStringLiteral("code_key/QR_btn_2.png")),
        skinPath(QStringLiteral("code_key/QR_btn_2_h.png")),
        skinPath(QStringLiteral("code_key/QR_btn_2.png")));
    connect(helpButton, &QPushButton::clicked, this, [this]() {
        if (!openDaoyuHomePage()) {
            showHint(QString::fromUtf8(u8"打开叨鱼官网失败。"));
        }
    });

    qrLabel_ = new QLabel(this);
    qrLabel_->setGeometry(84, 31, 104, 104);
    qrLabel_->setAlignment(Qt::AlignCenter);
    qrLabel_->setStyleSheet(QStringLiteral("QLabel{background:#ffffff;color:#222222;font:12px 'Microsoft YaHei';border:1px solid #dec28f;}"));

    stateIcon_ = new QLabel(this);
    stateIcon_->setGeometry(128, 41, 18, 18);
    setImageLabel(stateIcon_, skinPath(QStringLiteral("code_key/Vector.png")));

    const QString stateTextStyle = QStringLiteral("QLabel{background:transparent;color:#2b1a10;font:12px 'Microsoft YaHei';}");
    stateText1_ = new QLabel(this);
    stateText1_->setGeometry(85, 57, 100, 20);
    stateText1_->setAlignment(Qt::AlignCenter);
    stateText1_->setStyleSheet(stateTextStyle);

    stateText2_ = new QLabel(this);
    stateText2_->setGeometry(88, 75, 95, 20);
    stateText2_->setAlignment(Qt::AlignCenter);
    stateText2_->setStyleSheet(stateTextStyle);

    stateText3_ = new QLabel(this);
    stateText3_->setGeometry(89, 92, 95, 20);
    stateText3_->setAlignment(Qt::AlignCenter);
    stateText3_->setStyleSheet(stateTextStyle);

    refreshOverlayButton_ = new QPushButton(QString::fromUtf8(u8"刷新"), this);
    refreshOverlayButton_->setGeometry(111, 92, 50, 20);
    refreshOverlayButton_->setStyleSheet(QStringLiteral(
        "QPushButton{border:0;background:rgba(55,32,18,170);color:#ffeddc;font:12px 'Microsoft YaHei';}"
        "QPushButton:hover{background:rgba(95,52,22,210);}"));
    connect(refreshOverlayButton_, &QPushButton::clicked, this, [this]() {
        startNewFlowFromRefreshClick();
    });

    cancelScannedButton_ = new QPushButton(QString::fromUtf8(u8"取消"), this);
    cancelScannedButton_->setGeometry(111, 107, 50, 25);
    cancelScannedButton_->setStyleSheet(QStringLiteral(
        "QPushButton{border:0;background:transparent;color:#2f77e5;font:12px 'Microsoft YaHei';}"
        "QPushButton:hover{text-decoration:underline;}"));
    connect(cancelScannedButton_, &QPushButton::clicked, this, [this]() {
        startNewFlow();
    });
}

void CodeKeyLoginPanel::startNewFlow()
{
    if (!isProtocolAccepted()) {
        pollTimer_.stop();
        setState(CodeKeyLoginState::Idle);
        showHint(QString::fromUtf8(u8"请先勾选协议后再刷新二维码。"));
        return;
    }
    if (!authenticator_ || requestInFlight_) {
        return;
    }

    pollTimer_.stop();
    pollInFlight_ = false;
    requestInFlight_ = true;
    setState(CodeKeyLoginState::LoadingCode);

    QPointer<CodeKeyLoginPanel> guard(this);
    authenticator_->requestCodeImage(qrLabel_ ? qrLabel_->width() : 104, [guard](const CodeKeyImageResult& result) {
        auto* target = guard.data();
        if (!target) {
            return;
        }
        QMetaObject::invokeMethod(target, [guard, result]() {
            if (guard) {
                guard->handleCodeImageResult(result);
            }
        }, Qt::QueuedConnection);
    });
}

void CodeKeyLoginPanel::startNewFlowFromRefreshClick()
{
    const qint64 now = refreshClickClock_.isValid() ? refreshClickClock_.elapsed() : 0;
    if (!shouldAllowCodeKeyRefreshClick(
            lastAcceptedRefreshClickMs_,
            now,
            normalizedCodeKeyRefreshClickIntervalMs(options_.refreshClickIntervalMs))) {
        return;
    }
    lastAcceptedRefreshClickMs_ = now;
    startNewFlow();
}

void CodeKeyLoginPanel::schedulePoll()
{
    if (!active_ || requestInFlight_ || pollInFlight_) {
        return;
    }
    if (!isProtocolAccepted()) {
        return;
    }
    if (state_ == CodeKeyLoginState::CodeReady || state_ == CodeKeyLoginState::Scanned) {
        pollTimer_.start();
    }
}

void CodeKeyLoginPanel::pollOnce()
{
    if (!active_ || !authenticator_ || pollInFlight_) {
        return;
    }
    if (state_ != CodeKeyLoginState::CodeReady && state_ != CodeKeyLoginState::Scanned) {
        return;
    }
    if (!isProtocolAccepted()) {
        setState(CodeKeyLoginState::Idle);
        return;
    }

    pollInFlight_ = true;
    QPointer<CodeKeyLoginPanel> guard(this);
    authenticator_->pollLogin(qrLabel_ ? qrLabel_->width() : 104, [guard](const CodeKeyLoginResult& result) {
        auto* target = guard.data();
        if (!target) {
            return;
        }
        QMetaObject::invokeMethod(target, [guard, result]() {
            if (guard) {
                guard->handleLoginPollResult(result);
            }
        }, Qt::QueuedConnection);
    });
}

void CodeKeyLoginPanel::handleCodeImageResult(const CodeKeyImageResult& result)
{
    requestInFlight_ = false;
    if (!active_) {
        return;
    }
    if (!result.success) {
        setState(CodeKeyLoginState::Failed);
        showHint(QString::fromStdWString(result.errorMessage.empty() ? L"获取二维码失败。" : result.errorMessage));
        return;
    }

    QPixmap pixmap;
    if (!pixmap.loadFromData(result.imageBytes.data(), static_cast<uint>(result.imageBytes.size()))) {
        setState(CodeKeyLoginState::Failed);
        showHint(QString::fromUtf8(u8"二维码图片解析失败。"));
        return;
    }

    codePixmap_ = pixmap;
    qrLabel_->setPixmap(pixmap);
    qrLabel_->setScaledContents(true);
    setState(CodeKeyLoginState::CodeReady);
    schedulePoll();
}

void CodeKeyLoginPanel::handleLoginPollResult(const CodeKeyLoginResult& result)
{
    pollInFlight_ = false;
    if (!active_) {
        return;
    }

    const CodeKeyLoginState nextState = codeKeyStateForResult(result);
    if (nextState == CodeKeyLoginState::Success) {
        setState(CodeKeyLoginState::Success);
        if (options_.completionHandler) {
            options_.completionHandler(result);
        }
        return;
    }

    setState(nextState);
    if (shouldContinueCodeKeyPolling(result)) {
        schedulePoll();
        return;
    }

    if (!result.errorMessage.empty()) {
        showHint(QString::fromStdWString(result.errorMessage));
    }
}

void CodeKeyLoginPanel::setState(CodeKeyLoginState state)
{
    state_ = state;
    if (stateIcon_) {
        stateIcon_->hide();
    }
    if (stateText1_) {
        stateText1_->hide();
    }
    if (stateText2_) {
        stateText2_->hide();
    }
    if (stateText3_) {
        stateText3_->hide();
    }
    if (refreshOverlayButton_) {
        refreshOverlayButton_->hide();
    }
    if (cancelScannedButton_) {
        cancelScannedButton_->hide();
    }

    if (!qrLabel_) {
        return;
    }

    switch (state_) {
    case CodeKeyLoginState::Idle:
        qrLabel_->setPixmap(QPixmap());
        qrLabel_->setScaledContents(false);
        qrLabel_->setText(QString::fromUtf8(u8"请勾选协议"));
        break;
    case CodeKeyLoginState::LoadingCode:
        qrLabel_->setPixmap(QPixmap());
        qrLabel_->setScaledContents(false);
        qrLabel_->setText(QString::fromUtf8(u8"加载中..."));
        break;
    case CodeKeyLoginState::CodeReady:
        if (!codePixmap_.isNull()) {
            qrLabel_->setPixmap(codePixmap_);
            qrLabel_->setScaledContents(true);
        }
        qrLabel_->setText(QString());
        break;
    case CodeKeyLoginState::Scanned:
        setImageLabel(qrLabel_, skinPath(QStringLiteral("code_key/bg_success1.png")));
        if (stateIcon_) {
            stateIcon_->show();
        }
        if (stateText1_) {
            stateText1_->setText(QString::fromUtf8(u8"扫码成功"));
            stateText1_->show();
        }
        if (stateText2_) {
            stateText2_->setText(QString::fromUtf8(u8"请在手机端选择"));
            stateText2_->show();
        }
        if (stateText3_) {
            stateText3_->setText(QString::fromUtf8(u8"通行证登录"));
            stateText3_->show();
        }
        if (cancelScannedButton_) {
            cancelScannedButton_->show();
        }
        break;
    case CodeKeyLoginState::Expired:
        setImageLabel(qrLabel_, skinPath(QStringLiteral("code_key/bg_failed.png")));
        if (stateText1_) {
            stateText1_->setText(QString::fromUtf8(u8"二维码已失效"));
            stateText1_->show();
        }
        if (refreshOverlayButton_) {
            refreshOverlayButton_->show();
        }
        break;
    case CodeKeyLoginState::Failed:
        setImageLabel(qrLabel_, skinPath(QStringLiteral("code_key/bg.png")));
        if (stateText1_) {
            stateText1_->setText(QString::fromUtf8(u8"扫码登录失败"));
            stateText1_->show();
        }
        if (refreshOverlayButton_) {
            refreshOverlayButton_->show();
        }
        break;
    case CodeKeyLoginState::Success:
        break;
    }
}

void CodeKeyLoginPanel::showHint(const QString& text)
{
    if (options_.hintHandler) {
        options_.hintHandler(text);
    }
}

bool CodeKeyLoginPanel::isProtocolAccepted() const
{
    return !options_.protocolAccepted || options_.protocolAccepted();
}

bool CodeKeyLoginPanel::openDaoyuHomePage() const
{
    return QDesktopServices::openUrl(QUrl(configuredDaoyuUrl(options_.daoyuHomeUrl)));
}

}
