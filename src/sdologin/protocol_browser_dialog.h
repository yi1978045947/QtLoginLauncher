#pragma once

#include <QDialog>
#include <QString>

class QLabel;
class QTextBrowser;

namespace qtlogin::sdologin {

class ProtocolBrowserDialog final : public QDialog {
public:
    ProtocolBrowserDialog(const QString& title, const QString& url, QWidget* parent = nullptr);
    ProtocolBrowserDialog(const QString& title, const QString& url, double requestedDpiScale, QWidget* parent = nullptr);
    ProtocolBrowserDialog(const QString& title, const QString& url, double requestedDpiScale, int agreeDelaySeconds, QWidget* parent = nullptr);

private:
    QString fallbackErrorHtml(const QString& title, const QString& url, const QString& error) const;
    QString fallbackLoadingHtml(const QString& title, const QString& url) const;
    void loadFallbackUrl(QTextBrowser* browser, QLabel* status, const QString& title, const QString& url);
};

}
