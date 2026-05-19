#pragma once

#include <functional>
#include <vector>

#include <QLineEdit>
#include <QPointer>
#include <QString>
#include <QVector>
#include <QWidget>

#include "config_manager.h"

class QFrame;
class QPushButton;

namespace qtlogin::sdologin {

class AccountHistoryCombo final : public QWidget {
public:
    struct Options {
        QString skinRoot;
        QString placeholder;
        int popupFontPixelSize = 0;
        std::vector<qtlogin::config::ConfigManager::UserAccountEntry> accounts;
        std::function<void(const std::wstring&)> removeHandler;
    };

    explicit AccountHistoryCombo(Options options, QWidget* parent = nullptr);
    ~AccountHistoryCombo() override;

    QLineEdit* lineEdit() const { return edit_; }
    QString text() const;
    void setTextSilently(const QString& text);
    void setAccounts(std::vector<qtlogin::config::ConfigManager::UserAccountEntry> accounts);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QString skinPath(const QString& relative) const;
    QString resolveSkinPath(const QString& preferred, const QString& fallback) const;
    void layoutChildren();
    void togglePopup();
    void showPopup();
    void hidePopup();
    void rebuildPopup();
    void schedulePopupRefresh();
    void selectAccount(const QString& account);
    void removeAccount(const QString& account);

    QString skinRoot_;
    QString placeholder_;
    int popupFontPixelSize_ = 0;
    QVector<QString> accounts_;
    std::function<void(const std::wstring&)> removeHandler_;
    QLineEdit* edit_ = nullptr;
    QPushButton* dropButton_ = nullptr;
    QPointer<QFrame> popup_;
    bool popupRefreshPending_ = false;
};

}
