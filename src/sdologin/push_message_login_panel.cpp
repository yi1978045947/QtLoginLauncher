#include "push_message_login_panel.h"

#include <QDesktopServices>
#include <QDir>
#include <QHideEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QUrl>

#include <utility>

#include "account_history_combo.h"
#include "quick_login_skin.h"

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

QString editStyle(const QString& normalImage, const QString& focusImage)
{
    return QStringLiteral(
        "QLineEdit{border:0;border-image:%1;color:#ffe1b7;background:transparent;"
        "font:12px 'Microsoft YaHei';padding-left:10px;padding-right:28px;}"
        "QLineEdit:focus{border-image:%2;}"
        "QLineEdit::placeholder{color:#ffd1a4;}")
        .arg(imageUrl(normalImage), imageUrl(focusImage));
}

void applyButtonSpec(QPushButton* button, const SkinButtonSpec& spec, const std::function<QString(const std::string&)>& assetPath, bool applyGeometry)
{
    if (applyGeometry) {
        button->setGeometry(spec.x, spec.y, spec.width, spec.height);
    }
    QString style = QStringLiteral("QPushButton{border:0;border-image:%1;color:#ffffff;background:transparent;font:bold 20px 'Microsoft YaHei';}")
        .arg(imageUrl(assetPath(spec.normalImage)));
    if (!spec.hoverImage.empty()) {
        style += QStringLiteral("QPushButton:hover{border-image:%1;}").arg(imageUrl(assetPath(spec.hoverImage)));
    }
    if (!spec.pressedImage.empty()) {
        style += QStringLiteral("QPushButton:pressed{border-image:%1;}").arg(imageUrl(assetPath(spec.pressedImage)));
    }
    if (!spec.disabledImage.empty()) {
        style += QStringLiteral("QPushButton:disabled{border-image:%1;color:#ffe5cc;}").arg(imageUrl(assetPath(spec.disabledImage)));
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

PushMessageLoginPanel::PushMessageLoginPanel(PushMessageLoginPanelOptions options, QWidget* parent)
    : QWidget(parent)
    , options_(std::move(options))
    , authenticator_(std::make_unique<SdoBasePushMessageAuthenticator>(options_.authConfig))
    , retryTimer_(this)
    , pollTimer_(this)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("background:transparent;"));
    resize(442, 190);

    retryTimer_.setInterval(1000);
    connect(&retryTimer_, &QTimer::timeout, this, [this]() {
        int remaining = loginButton_ ? loginButton_->property("retrySeconds").toInt() - 1 : 0;
        if (!loginButton_) {
            retryTimer_.stop();
            return;
        }
        loginButton_->setProperty("retrySeconds", remaining);
        if (remaining <= 0) {
            retryTimer_.stop();
            loginButton_->setEnabled(true);
            loginButton_->setText(QString());
            return;
        }
        loginButton_->setText(QString::fromUtf8(u8"%1秒后重试").arg(remaining));
    });

    pollTimer_.setSingleShot(true);
    pollTimer_.setInterval(1000);
    connect(&pollTimer_, &QTimer::timeout, this, [this]() {
        pollOnce();
    });

    buildUi();
}

PushMessageLoginPanel::~PushMessageLoginPanel()
{
    retryTimer_.stop();
    pollTimer_.stop();
    if (authenticator_) {
        authenticator_->cancel();
    }
}

void PushMessageLoginPanel::hideEvent(QHideEvent* event)
{
    retryTimer_.stop();
    pollTimer_.stop();
    sendInFlight_ = false;
    pollInFlight_ = false;
    if (authenticator_) {
        authenticator_->cancel();
    }
    QWidget::hideEvent(event);
}

QString PushMessageLoginPanel::skinPath(const QString& relative) const
{
    return QDir(options_.skinRoot).filePath(relative);
}

void PushMessageLoginPanel::buildUi()
{
    const QuickLoginSkinSpec skin = quickLoginSkinSpec();
    const auto skinAsset = [this](const std::string& relative) {
        return skinPath(QString::fromStdString(relative));
    };

    auto* download = new QPushButton(QString::fromUtf8(u8"需安装叨鱼"), this);
    download->setGeometry(96, 22, 177, 22);
    download->setCursor(Qt::PointingHandCursor);
    download->setStyleSheet(QStringLiteral(
        "QPushButton{border:0;color:#ffeddc;background:transparent;text-align:center;font:bold 14px 'Microsoft YaHei';}"
        "QPushButton:hover{text-decoration:underline;color:#ffffff;}"));
    connect(download, &QPushButton::clicked, this, [this]() {
        if (!openDaoyuHomePage()) {
            showHint(QString::fromUtf8(u8"打开叨鱼官网失败。"));
        }
    });

    auto* accountIcon = new QLabel(this);
    accountIcon->setGeometry(53, 53, 40, 22);
    setImageLabel(accountIcon, skinPath(QStringLiteral("login/account.png")));

    AccountHistoryCombo::Options accountOptions;
    accountOptions.skinRoot = options_.skinRoot;
    accountOptions.placeholder = options_.accountPlaceholder;
    accountOptions.popupFontPixelSize = options_.accountPopupFontPixelSize;
    accountOptions.accounts = options_.accountHistory;
    accountOptions.removeHandler = options_.removeAccountHandler;
    auto* accountCombo = new AccountHistoryCombo(std::move(accountOptions), this);
    accountCombo->setGeometry(96, 51, 177, 29);
    accountEdit_ = accountCombo->lineEdit();
    accountEdit_->setPlaceholderText(options_.accountPlaceholder.isEmpty()
            ? QString::fromUtf8(u8"手机/邮箱/个性账号")
            : options_.accountPlaceholder);
    accountEdit_->setStyleSheet(editStyle(
        skinPath(QStringLiteral("combobox/combo_n.png")),
        skinPath(QStringLiteral("combobox/combo_o.png"))));

    statusText_ = new QLabel(this);
    statusText_->setGeometry(96, 88, 220, 36);
    statusText_->setStyleSheet(QStringLiteral("QLabel{color:#ffeddc;background:transparent;font:16px 'Microsoft YaHei';}"));
    statusText_->setVisible(false);

    loginButton_ = new QPushButton(this);
    applyButtonSpec(
        loginButton_,
        skin.actionButton,
        skinAsset,
        shouldApplyPushMessageButtonGeometryForSkinChange(true));
    loginButton_->setText(QString());
    connect(loginButton_, &QPushButton::clicked, this, &PushMessageLoginPanel::startLogin);
}

void PushMessageLoginPanel::startLogin()
{
    if (sendInFlight_) {
        return;
    }

    PushMessageLoginRequest request;
    request.account = accountEdit_ ? accountEdit_->text().toStdWString() : std::wstring();
    request.protocolAccepted = isProtocolAccepted();
    request.supportMobileAccountOnly = options_.supportMobileAccountOnly;

    const PushMessageLoginValidationResult validation = validatePushMessageLoginRequest(request);
    if (validation.code != PushMessageLoginValidationCode::Ok) {
        showHint(QString::fromStdWString(validation.message));
        return;
    }

    retryTimer_.stop();
    pollTimer_.stop();
    pollInFlight_ = false;
    sendInFlight_ = true;
    setStatusText(QString::fromUtf8(u8"正在发送一键登录确认..."));
    if (loginButton_) {
        loginButton_->setEnabled(false);
        loginButton_->setText(QString());
    }

    QPointer<PushMessageLoginPanel> guard(this);
    authenticator_->startLogin(request, [guard](const PushMessageSendResult& result) {
        auto* target = guard.data();
        if (!target) {
            return;
        }
        QMetaObject::invokeMethod(target, [guard, result]() {
            if (guard) {
                guard->handleSendResult(result);
            }
        }, Qt::QueuedConnection);
    });
}

void PushMessageLoginPanel::handleSendResult(const PushMessageSendResult& result)
{
    sendInFlight_ = false;
    if (!result.success) {
        restoreActionButton();
        setStatusText(QString(), false);
        showHint(QString::fromStdWString(result.errorMessage.empty() ? L"发送一键登录确认失败，请检查网络连接。" : result.errorMessage));
        return;
    }

    const QString serialText = result.serialNumber.empty()
        ? QString::fromUtf8(u8"一键登录确认码已发送")
        : QString::fromUtf8(u8"登录确认码: %1").arg(QString::fromStdString(result.serialNumber));
    setStatusText(serialText);
    setButtonRetryCountdown();
    schedulePoll();
}

void PushMessageLoginPanel::schedulePoll()
{
    if (sendInFlight_ || pollInFlight_) {
        return;
    }
    pollTimer_.start();
}

void PushMessageLoginPanel::pollOnce()
{
    if (!authenticator_ || sendInFlight_ || pollInFlight_) {
        return;
    }
    pollInFlight_ = true;
    QPointer<PushMessageLoginPanel> guard(this);
    authenticator_->pollLogin([guard](const PushMessageLoginResult& result) {
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

void PushMessageLoginPanel::handleLoginPollResult(const PushMessageLoginResult& result)
{
    pollInFlight_ = false;
    const PushMessageLoginState state = pushMessageStateForLoginResult(result);
    if (state == PushMessageLoginState::Success) {
        retryTimer_.stop();
        pollTimer_.stop();
        if (options_.successfulAccountHandler && accountEdit_) {
            options_.successfulAccountHandler(accountEdit_->text().trimmed());
        }
        if (options_.completionHandler) {
            options_.completionHandler(result);
        }
        return;
    }

    if (shouldContinuePushMessagePolling(result)) {
        setStatusText(QString::fromUtf8(u8"请打开手机叨鱼确认登录"));
        schedulePoll();
        return;
    }

    pollTimer_.stop();
    restoreActionButton();
    showHint(QString::fromStdWString(result.errorMessage.empty() ? L"一键登录失败，请重试。" : result.errorMessage));
}

void PushMessageLoginPanel::setButtonRetryCountdown()
{
    if (!loginButton_) {
        return;
    }
    const QuickLoginSkinSpec skin = quickLoginSkinSpec();
    const auto skinAsset = [this](const std::string& relative) {
        return skinPath(QString::fromStdString(relative));
    };
    applyButtonSpec(
        loginButton_,
        skin.retryButton,
        skinAsset,
        shouldApplyPushMessageButtonGeometryForSkinChange(false));
    loginButton_->setEnabled(false);
    const int seconds = normalizedPushMessageRetryCountdownSeconds(skin.retryCountdownSeconds);
    loginButton_->setProperty("retrySeconds", seconds);
    loginButton_->setText(QString::fromUtf8(u8"%1秒后重试").arg(seconds));
    retryTimer_.start();
}

void PushMessageLoginPanel::restoreActionButton()
{
    retryTimer_.stop();
    const QuickLoginSkinSpec skin = quickLoginSkinSpec();
    const auto skinAsset = [this](const std::string& relative) {
        return skinPath(QString::fromStdString(relative));
    };
    if (loginButton_) {
        applyButtonSpec(
            loginButton_,
            skin.actionButton,
            skinAsset,
            shouldApplyPushMessageButtonGeometryForSkinChange(false));
        loginButton_->setEnabled(true);
        loginButton_->setText(QString());
    }
}

void PushMessageLoginPanel::setStatusText(const QString& text, bool visible)
{
    if (!statusText_) {
        return;
    }
    statusText_->setText(text);
    statusText_->setVisible(visible && !text.isEmpty());
}

void PushMessageLoginPanel::showHint(const QString& text)
{
    if (options_.hintHandler) {
        options_.hintHandler(text);
    }
}

bool PushMessageLoginPanel::isProtocolAccepted() const
{
    return !options_.protocolAccepted || options_.protocolAccepted();
}

bool PushMessageLoginPanel::openDaoyuHomePage() const
{
    return QDesktopServices::openUrl(QUrl(configuredDaoyuUrl(options_.daoyuHomeUrl)));
}

}
