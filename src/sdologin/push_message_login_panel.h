#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QTimer>
#include <QString>
#include <QWidget>

#include "push_message_login_model.h"
#include "config_manager.h"
#include "sdo_base_push_message_auth.h"

class QLabel;
class QLineEdit;
class QPushButton;

namespace qtlogin::sdologin {

struct PushMessageLoginPanelOptions {
    SdoBaseAuthConfig authConfig;
    QString skinRoot;
    QString accountPlaceholder;
    std::wstring daoyuHomeUrl;
    int accountPopupFontPixelSize = 0;
    bool supportMobileAccountOnly = false;
    std::function<bool()> protocolAccepted;
    std::function<void(const QString&)> hintHandler;
    std::function<void(const QString&)> successfulAccountHandler;
    std::function<void(const std::wstring&)> removeAccountHandler;
    std::function<void(const PushMessageLoginResult&)> completionHandler;
    std::vector<qtlogin::config::ConfigManager::UserAccountEntry> accountHistory;
};

class PushMessageLoginPanel final : public QWidget {
public:
    explicit PushMessageLoginPanel(PushMessageLoginPanelOptions options, QWidget* parent = nullptr);
    ~PushMessageLoginPanel();
    QLineEdit* accountEdit() const { return accountEdit_; }

protected:
    void hideEvent(QHideEvent* event) override;

private:
    QString skinPath(const QString& relative) const;
    void buildUi();
    void startLogin();
    void handleSendResult(const PushMessageSendResult& result);
    void schedulePoll();
    void pollOnce();
    void handleLoginPollResult(const PushMessageLoginResult& result);
    void setButtonRetryCountdown();
    void restoreActionButton();
    void setStatusText(const QString& text, bool visible = true);
    void showHint(const QString& text);
    bool isProtocolAccepted() const;
    bool openDaoyuHomePage() const;

    PushMessageLoginPanelOptions options_;
    std::unique_ptr<SdoBasePushMessageAuthenticator> authenticator_;
    QLineEdit* accountEdit_ = nullptr;
    QLabel* statusText_ = nullptr;
    QPushButton* loginButton_ = nullptr;
    QTimer retryTimer_;
    QTimer pollTimer_;
    bool sendInFlight_ = false;
    bool pollInFlight_ = false;
};

}
