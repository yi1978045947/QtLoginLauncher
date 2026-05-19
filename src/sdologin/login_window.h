#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QPoint>
#include <QPointer>
#include <QString>
#include <QWidget>

#include "client_config_model.h"
#include "code_key_login_model.h"
#include "config_manager.h"
#include "push_message_login_model.h"
#include "sdo_base_password_auth.h"

class QLineEdit;
class QPushButton;

namespace qtlogin::sdologin {

class CodeKeyLoginPanel;
class PushMessageLoginPanel;
class AccountHistoryCombo;

struct LoginWindowContext {
    std::wstring appName;
    std::wstring appVersion;
    int appId = 0;
    int areaId = 0;
    int groupId = 0;
    void* ownerHwnd = nullptr;
    bool embedWindow = false;
    int posX = 0;
    int posY = 0;
    bool quickLoginEnabled = true;
    std::wstring skinType;
    std::wstring privacyPolicyUrl;
    std::wstring serviceAgreementUrl;
    std::wstring supplementaryRulesUrl;
    std::wstring daoyuHomeUrl;
    std::wstring forgotPasswordUrl;
    std::wstring udpPassport;
    std::wstring tcpPassport;
    bool verifyCert = false;
    bool standalone = false;
    double legacyDpiScaleFactor = 0.0;
    int savedPrivacyPolicyVersion = -1;
    int savedServiceAgreementVersion = -1;
    bool protocolReviewRequired = false;
    int protocolAgreeDelaySeconds = 5;
    int acceptedPrivacyPolicyVersion = -1;
    int acceptedServiceAgreementVersion = -1;
    bool fetchClientConfig = true;
    bool hasServerClientConfig = false;
    int codeKeyRefreshClickIntervalMs = 1500;
    int accountHistoryMaxCount = 10;
    std::vector<qtlogin::config::ConfigManager::UserAccountEntry> accountHistory;
    qtlogin::config::ClientConfigModel serverClientConfig;
};

struct LoginWindowResult {
    bool success = false;
    std::string sessionId;
    std::string sndaid;
    std::string identityState;
    std::string ticket;
};

class LoginWindow final : public QWidget {
public:
    explicit LoginWindow(const LoginWindowContext& context, QWidget* parent = nullptr);

    void setCompletionHandler(std::function<void(const LoginWindowResult&)> handler);
    void triggerAutoSuccess();
    void applyEmbedding();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    QString skinPath(const QString& relative) const;
    void buildUi();
    void buildPasswordPage(QWidget* page);
    void buildSmsPage(QWidget* page);
    CodeKeyLoginPanel* buildQrPage(QWidget* page);
    void buildQuickLoginPage(QWidget* page);
    void selectPage(int index);
    void completeSuccess(const QString& source);
    void completePasswordLogin(const PasswordAuthResult& authResult);
    void completeCodeKeyLogin(const CodeKeyLoginResult& authResult);
    void completePushMessageLogin(const PushMessageLoginResult& authResult);
    void completeCancel();
    void submitPasswordLogin(QLineEdit* accountEdit, QLineEdit* passwordEdit, QPushButton* loginButton);
    void showKeyboardForPassword(QLineEdit* passwordEdit, QPushButton* loginButton);
    void showProtocolDialog(
        const QString& title,
        const QString& url,
        int agreeDelaySeconds = 0,
        bool recordInitialAcceptance = false);
    void prewarmProtocolBrowser();
    void showLegacyDialogGallery();
    void showHint(const QString& text);
    void setProtocolDialogActive(bool active);
    bool persistInitialProtocolAcceptance();
    void recordSuccessfulAccount(const QString& account, int type, bool refreshVisibleUi);
    void removeHistoryAccount(const std::wstring& account);
    void registerSharedAccountEdit(QLineEdit* edit);
    void updateSharedAccountText(QLineEdit* source, const QString& text);

    LoginWindowContext context_;
    QString skinRoot_;
    QPixmap background_;
    QPoint dragOffset_;
    double legacyDpiScaleFactor_ = 1.0;
    bool completed_ = false;
    bool initialPrivacyProtocolOpened_ = false;
    bool protocolDialogActive_ = false;
    QString lastSubmittedPasswordAccount_;
    QString lastSubmittedPushAccount_;
    QString sharedAccountText_;
    std::vector<QPointer<QLineEdit>> sharedAccountEdits_;
    std::function<void(const LoginWindowResult&)> completionHandler_;
    std::unique_ptr<SdoBasePasswordAuthenticator> passwordAuthenticator_;
    QWidget* keyboardDialog_ = nullptr;
};

}
