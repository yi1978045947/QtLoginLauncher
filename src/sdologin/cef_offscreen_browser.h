#pragma once

#include <QImage>
#include <QString>
#include <QWidget>

#include <functional>

#include <windows.h>

#include "include/cef_browser.h"
#include "include/cef_client.h"

namespace qtlogin::sdologin {

int executeCefSubProcess(HINSTANCE instance);
bool ensureCefInitialized();
void shutdownCef();

class CefWidgetClient;

class CefOffscreenBrowserWidget final : public QWidget {
public:
    explicit CefOffscreenBrowserWidget(QWidget* parent = nullptr);
    ~CefOffscreenBrowserWidget() override;

    void loadUrl(const QString& url);
    void loadFallbackHtml(const QString& title, const QString& url);
    void sendMessageToWeb(const QString& name, const QString& payload);
    void setWebMessageHandler(std::function<void(const QString&, const QString&)> handler);
    void handleWebMessageFromCef(const std::string& name, const std::string& payload);
    void setBrowser(CefRefPtr<CefBrowser> browser);
    void updateImage(const QImage& image);
    QSize browserSize() const;
    bool hasContentPainted() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void createBrowserIfNeeded();
    void sendMouseClick(QMouseEvent* event, bool mouseUp);
    void invalidateView();

    QString pendingUrl_;
    QImage image_;
    bool contentPainted_ = false;
    CefRefPtr<CefBrowser> browser_;
    CefRefPtr<CefWidgetClient> client_;
    std::function<void(const QString&, const QString&)> webMessageHandler_;
};

}
