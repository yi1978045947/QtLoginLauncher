#pragma once

#include <functional>
#include <memory>
#include <string>

#include <QPixmap>
#include <QElapsedTimer>
#include <QString>
#include <QTimer>
#include <QWidget>

#include "sdo_base_code_key_auth.h"

class QLabel;
class QPushButton;

namespace qtlogin::sdologin {

struct CodeKeyLoginPanelOptions {
    QString skinRoot;
    SdoBaseAuthConfig authConfig;
    std::wstring daoyuHomeUrl;
    int refreshClickIntervalMs = 1500;
    std::function<bool()> protocolAccepted;
    std::function<void(const QString&)> hintHandler;
    std::function<void(const CodeKeyLoginResult&)> completionHandler;
};

class CodeKeyLoginPanel final : public QWidget {
public:
    explicit CodeKeyLoginPanel(CodeKeyLoginPanelOptions options, QWidget* parent = nullptr);
    ~CodeKeyLoginPanel() override;

    void activate();
    void deactivate();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    QString skinPath(const QString& relative) const;
    void buildUi();
    void startNewFlow();
    void startNewFlowFromRefreshClick();
    void schedulePoll();
    void pollOnce();
    void handleCodeImageResult(const CodeKeyImageResult& result);
    void handleLoginPollResult(const CodeKeyLoginResult& result);
    void setState(CodeKeyLoginState state);
    void showHint(const QString& text);
    bool isProtocolAccepted() const;
    bool openDaoyuHomePage() const;

    CodeKeyLoginPanelOptions options_;
    std::unique_ptr<SdoBaseCodeKeyAuthenticator> authenticator_;
    QTimer pollTimer_;
    CodeKeyLoginState state_ = CodeKeyLoginState::Idle;
    bool active_ = false;
    bool requestInFlight_ = false;
    bool pollInFlight_ = false;
    QElapsedTimer refreshClickClock_;
    qint64 lastAcceptedRefreshClickMs_ = -1;
    QPixmap codePixmap_;
    QLabel* qrLabel_ = nullptr;
    QLabel* stateIcon_ = nullptr;
    QLabel* stateText1_ = nullptr;
    QLabel* stateText2_ = nullptr;
    QLabel* stateText3_ = nullptr;
    QPushButton* refreshTopButton_ = nullptr;
    QPushButton* refreshOverlayButton_ = nullptr;
    QPushButton* cancelScannedButton_ = nullptr;
};

}
