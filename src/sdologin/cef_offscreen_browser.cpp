#include "cef_offscreen_browser.h"

#include <QCoreApplication>
#include <QDir>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QUrl>
#include <QWheelEvent>

#include <atomic>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <utility>

#include "include/cef_app.h"
#include "include/cef_command_line.h"
#include "include/cef_process_message.h"
#include "include/cef_request.h"
#include "include/cef_request_handler.h"
#include "include/cef_render_handler.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"
#include "cef_web_bridge.h"
#include "protocol_browser_style.h"

#include <shellapi.h>

namespace qtlogin::sdologin {

namespace {

std::wstring executableDirectoryWide()
{
    std::vector<wchar_t> buffer(MAX_PATH);
    DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (size == buffer.size()) {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    if (size == 0) {
        return L".";
    }

    std::wstring path(buffer.data(), size);
    const size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L".";
    }
    return path.substr(0, pos);
}

void logCef(const std::wstring& message)
{
    std::wstring line = L"qtlogin cef: " + message + L"\r\n";
    ::OutputDebugStringW(line.c_str());

    const std::wstring path = executableDirectoryWide() + L"\\qtlogin_rewrite.log";
    std::wofstream file(path, std::ios::app);
    if (file.is_open()) {
        file << line;
    }
}

std::wstring cefText(const CefString& text)
{
    return text.ToWString();
}

int sampledPixelDifferenceCount(const QImage& image)
{
    if (image.isNull() || image.width() < 16 || image.height() < 16) {
        return 0;
    }

    const QColor base = image.pixelColor(0, 0);
    int different = 0;
    for (int y = 0; y < image.height(); y += 8) {
        for (int x = 0; x < image.width(); x += 8) {
            const QColor color = image.pixelColor(x, y);
            const int delta = std::abs(color.red() - base.red()) +
                std::abs(color.green() - base.green()) +
                std::abs(color.blue() - base.blue()) +
                std::abs(color.alpha() - base.alpha());
            if (delta > 36) {
                ++different;
            }
        }
    }
    return different;
}

bool imageHasUsefulContent(const QImage& image)
{
    return sampledPixelDifferenceCount(image) >= 20;
}

QString qStringFromUtf8String(const std::string& value)
{
    return QString::fromUtf8(value.c_str(), static_cast<int>(value.size()));
}

std::string utf8StringFromQString(const QString& value)
{
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<size_t>(utf8.size()));
}

std::wstring wideStringFromQString(const QString& value)
{
    if (value.isEmpty()) {
        return std::wstring();
    }
    const ushort* utf16 = value.utf16();
    const auto* begin = reinterpret_cast<const wchar_t*>(utf16);
    return std::wstring(begin, begin + value.size());
}

std::wstring wideStringFromUtf8(const std::string& value)
{
    if (value.empty()) {
        return std::wstring();
    }
    const int wideLength = MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        nullptr,
        0);
    if (wideLength <= 0) {
        return std::wstring(value.begin(), value.end());
    }
    std::wstring result(static_cast<size_t>(wideLength), L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        result.data(),
        wideLength);
    return result;
}

void openExternalProtocolLink(const std::string& url)
{
    const std::wstring wideUrl = wideStringFromUtf8(url);
    if (wideUrl.empty()) {
        return;
    }
    logCef(L"open external protocol link " + wideUrl);
    ShellExecuteW(nullptr, L"open", wideUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

class LoadUrlTask final : public CefTask {
public:
    LoadUrlTask(CefRefPtr<CefBrowser> browser, std::string url)
        : browser_(browser), url_(std::move(url))
    {
    }

    void Execute() override
    {
        if (browser_) {
            browser_->GetMainFrame()->LoadURL(url_);
        }
    }

private:
    CefRefPtr<CefBrowser> browser_;
    std::string url_;

    IMPLEMENT_REFCOUNTING(LoadUrlTask);
};

void loadUrlOnCefUi(CefRefPtr<CefBrowser> browser, const std::string& url)
{
    CefPostTask(TID_UI, new LoadUrlTask(browser, url));
}

struct BridgeCallback {
    CefRefPtr<CefV8Context> context;
    CefRefPtr<CefV8Value> callback;
};

std::map<int, BridgeCallback> g_bridgeCallbacks;

class QtLoginV8BridgeHandler final : public CefV8Handler {
public:
    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value>,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override
    {
        CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
        CefRefPtr<CefBrowser> browser = context ? context->GetBrowser() : nullptr;
        if (!browser) {
            exception = "qtlogin bridge has no browser context";
            return true;
        }

        const std::string functionName = name.ToString();
        if (functionName == cefBridgePostMessageFunctionName()) {
            std::string messageName;
            std::string payload;
            if (!arguments.empty() && arguments[0]->IsString()) {
                messageName = arguments[0]->GetStringValue();
            }
            if (arguments.size() > 1 && arguments[1]->IsString()) {
                payload = arguments[1]->GetStringValue();
            }

            CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(cefBridgeWebToNativeMessageName());
            CefRefPtr<CefListValue> list = message->GetArgumentList();
            list->SetString(0, messageName);
            list->SetString(1, payload);
            browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, message);
            retval = CefV8Value::CreateBool(true);
            return true;
        }

        if (functionName == cefBridgeOnMessageFunctionName()) {
            if (arguments.empty() || !arguments[0]->IsFunction()) {
                exception = "qtlogin.onMessage expects a function";
                return true;
            }
            g_bridgeCallbacks[browser->GetIdentifier()] = {context, arguments[0]};
            retval = CefV8Value::CreateBool(true);
            return true;
        }

        return false;
    }

private:
    IMPLEMENT_REFCOUNTING(QtLoginV8BridgeHandler);
};

class QtLoginCefApp final : public CefApp,
                            public CefRenderProcessHandler {
public:
    QtLoginCefApp() = default;

    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }

    void OnBeforeCommandLineProcessing(const CefString&, CefRefPtr<CefCommandLine> commandLine) override
    {
        commandLine->AppendSwitch("disable-gpu-compositing");
        commandLine->AppendSwitch("disable-smooth-scrolling");
    }

    void OnContextCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame>,
                          CefRefPtr<CefV8Context> context) override
    {
        if (!browser || !context) {
            return;
        }

        CefRefPtr<CefV8Value> global = context->GetGlobal();
        CefRefPtr<CefV8Value> bridge = CefV8Value::CreateObject(nullptr, nullptr);
        CefRefPtr<CefV8Handler> handler = new QtLoginV8BridgeHandler();
        bridge->SetValue(
            cefBridgePostMessageFunctionName(),
            CefV8Value::CreateFunction(cefBridgePostMessageFunctionName(), handler),
            V8_PROPERTY_ATTRIBUTE_NONE);
        bridge->SetValue(
            cefBridgeOnMessageFunctionName(),
            CefV8Value::CreateFunction(cefBridgeOnMessageFunctionName(), handler),
            V8_PROPERTY_ATTRIBUTE_NONE);
        bridge->SetValue("version", CefV8Value::CreateString("cef109"), V8_PROPERTY_ATTRIBUTE_READONLY);
        global->SetValue(cefBridgeObjectName(), bridge, V8_PROPERTY_ATTRIBUTE_NONE);
    }

    void OnContextReleased(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame>,
                           CefRefPtr<CefV8Context>) override
    {
        if (browser) {
            g_bridgeCallbacks.erase(browser->GetIdentifier());
        }
    }

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame>,
                                  CefProcessId,
                                  CefRefPtr<CefProcessMessage> message) override
    {
        if (!browser || !message || message->GetName().ToString() != cefBridgeNativeToWebMessageName()) {
            return false;
        }

        auto callbackIt = g_bridgeCallbacks.find(browser->GetIdentifier());
        if (callbackIt == g_bridgeCallbacks.end() || !callbackIt->second.context || !callbackIt->second.callback) {
            return true;
        }

        CefRefPtr<CefListValue> list = message->GetArgumentList();
        const std::string messageName = list && list->GetSize() > 0 ? list->GetString(0) : std::string();
        const std::string payload = list && list->GetSize() > 1 ? list->GetString(1) : std::string();

        CefV8ValueList args;
        args.push_back(CefV8Value::CreateString(messageName));
        args.push_back(CefV8Value::CreateString(payload));
        callbackIt->second.context->Enter();
        callbackIt->second.callback->ExecuteFunction(nullptr, args);
        callbackIt->second.context->Exit();
        return true;
    }

private:
    IMPLEMENT_REFCOUNTING(QtLoginCefApp);
};

std::once_flag g_cefInitFlag;
std::atomic<bool> g_cefInitialized{false};

std::wstring cefCachePath()
{
    return executableDirectoryWide() + L"\\cef_cache";
}

int toCefEventFlags(Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    int flags = EVENTFLAG_NONE;
    if (buttons.testFlag(Qt::LeftButton)) {
        flags |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    }
    if (buttons.testFlag(Qt::MiddleButton)) {
        flags |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    }
    if (buttons.testFlag(Qt::RightButton)) {
        flags |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
    }
    if (modifiers.testFlag(Qt::ShiftModifier)) {
        flags |= EVENTFLAG_SHIFT_DOWN;
    }
    if (modifiers.testFlag(Qt::ControlModifier)) {
        flags |= EVENTFLAG_CONTROL_DOWN;
    }
    if (modifiers.testFlag(Qt::AltModifier)) {
        flags |= EVENTFLAG_ALT_DOWN;
    }
    return flags;
}

CefMouseEvent toCefMouseEvent(QMouseEvent* event)
{
    CefMouseEvent cefEvent;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    cefEvent.x = static_cast<int>(event->position().x());
    cefEvent.y = static_cast<int>(event->position().y());
#else
    cefEvent.x = event->x();
    cefEvent.y = event->y();
#endif
    cefEvent.modifiers = toCefEventFlags(event->buttons(), event->modifiers());
    return cefEvent;
}

CefMouseEvent toCefMouseEvent(QWheelEvent* event)
{
    CefMouseEvent cefEvent;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    cefEvent.x = static_cast<int>(event->position().x());
    cefEvent.y = static_cast<int>(event->position().y());
#else
    cefEvent.x = event->x();
    cefEvent.y = event->y();
#endif
    cefEvent.modifiers = toCefEventFlags(event->buttons(), event->modifiers());
    return cefEvent;
}

CefBrowserHost::MouseButtonType toCefButton(Qt::MouseButton button)
{
    switch (button) {
    case Qt::RightButton:
        return MBT_RIGHT;
    case Qt::MiddleButton:
        return MBT_MIDDLE;
    case Qt::LeftButton:
    default:
        return MBT_LEFT;
    }
}

}

class CefWidgetClient final : public CefClient,
                              public CefLifeSpanHandler,
                              public CefRenderHandler,
                              public CefLoadHandler,
                              public CefRequestHandler {
public:
    CefWidgetClient(CefOffscreenBrowserWidget* widget, std::string initialUrl)
        : widget_(widget), initialUrl_(std::move(initialUrl))
    {
    }

    void detach()
    {
        widget_ = nullptr;
    }

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser>,
                                  CefRefPtr<CefFrame>,
                                  CefProcessId,
                                  CefRefPtr<CefProcessMessage> message) override
    {
        if (!message || message->GetName().ToString() != cefBridgeWebToNativeMessageName()) {
            return false;
        }
        CefRefPtr<CefListValue> list = message->GetArgumentList();
        const std::string messageName = list && list->GetSize() > 0 ? list->GetString(0) : std::string();
        const std::string payload = list && list->GetSize() > 1 ? list->GetString(1) : std::string();
        if (widget_) {
            QMetaObject::invokeMethod(widget_, [widget = widget_, messageName, payload]() {
                widget->handleWebMessageFromCef(messageName, payload);
            }, Qt::QueuedConnection);
        }
        return true;
    }

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
    {
        logCef(L"OnAfterCreated");
        browser->GetHost()->WasHidden(false);
        browser->GetHost()->SetFocus(true);
        browser->GetHost()->WasResized();
        if (!initialUrl_.empty()) {
            logCef(L"CEF UI LoadURL " + std::wstring(initialUrl_.begin(), initialUrl_.end()));
            browser->GetMainFrame()->LoadURL(initialUrl_);
        }
        if (!widget_) {
            return;
        }
        QMetaObject::invokeMethod(widget_, [widget = widget_, browser]() {
            widget->setBrowser(browser);
        }, Qt::QueuedConnection);
    }

    bool OnBeforePopup(CefRefPtr<CefBrowser>,
                       CefRefPtr<CefFrame>,
                       const CefString& target_url,
                       const CefString&,
                       CefLifeSpanHandler::WindowOpenDisposition,
                       bool user_gesture,
                       const CefPopupFeatures&,
                       CefWindowInfo&,
                       CefRefPtr<CefClient>&,
                       CefBrowserSettings&,
                       CefRefPtr<CefDictionaryValue>&,
                       bool*) override
    {
        if (!user_gesture) {
            return false;
        }

        const std::string target = protocolExternalLinkTarget(initialUrl_, target_url.ToString());
        if (target.empty()) {
            return false;
        }

        openExternalProtocolLink(target);
        return true;
    }

    bool OnBeforeBrowse(CefRefPtr<CefBrowser>,
                        CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefRequest> request,
                        bool user_gesture,
                        bool) override
    {
        if (!frame || !frame->IsMain() || !request || !user_gesture) {
            return false;
        }

        const std::string target = protocolExternalLinkTarget(initialUrl_, request->GetURL().ToString());
        if (target.empty() || target == initialUrl_) {
            return false;
        }

        openExternalProtocolLink(target);
        return true;
    }

    void GetViewRect(CefRefPtr<CefBrowser>, CefRect& rect) override
    {
        const QSize size = widget_ ? widget_->browserSize() : QSize(1, 1);
        rect = CefRect(0, 0, std::max(1, size.width()), std::max(1, size.height()));
    }

    void OnPaint(CefRefPtr<CefBrowser>,
                 PaintElementType type,
                 const RectList&,
                 const void* buffer,
                 int width,
                 int height) override
    {
        if (!widget_ || type != PET_VIEW || width <= 0 || height <= 0) {
            return;
        }

        if (paintLogCount_ < 3) {
            logCef(L"OnPaint view " + std::to_wstring(width) + L"x" + std::to_wstring(height));
            ++paintLogCount_;
        }
        QByteArray pixels(static_cast<const char*>(buffer), width * height * 4);
        QMetaObject::invokeMethod(widget_, [widget = widget_, pixels = std::move(pixels), width, height]() mutable {
            const QImage image(reinterpret_cast<const uchar*>(pixels.constData()), width, height, QImage::Format_ARGB32);
            widget->updateImage(image.copy());
        }, Qt::QueuedConnection);
    }

    void OnLoadStart(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame> frame, TransitionType) override
    {
        if (frame && frame->IsMain()) {
            logCef(L"OnLoadStart " + cefText(frame->GetURL()));
        }
    }

    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override
    {
        if (frame && frame->IsMain()) {
            logCef(L"OnLoadEnd status=" + std::to_wstring(httpStatusCode) + L" " + cefText(frame->GetURL()));
            if (browser) {
                browser->GetHost()->WasResized();
                browser->GetHost()->Invalidate(PET_VIEW);
            }
        }
    }

    void OnLoadError(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     ErrorCode errorCode,
                     const CefString& errorText,
                     const CefString& failedUrl) override
    {
        if (!frame->IsMain()) {
            return;
        }
        logCef(L"OnLoadError " + cefText(errorText) + L" " + cefText(failedUrl));
        if (!shouldShowProtocolLoadError(static_cast<int>(errorCode))) {
            return;
        }
        const std::string errorStd = errorText.ToString();
        const std::string failedUrlStd = failedUrl.ToString();
        const QString text = QString::fromUtf8(errorStd.c_str(), static_cast<int>(errorStd.size())).toHtmlEscaped();
        const QString url = QString::fromUtf8(failedUrlStd.c_str(), static_cast<int>(failedUrlStd.size())).toHtmlEscaped();
        const QString html = QStringLiteral(
                                 "<html><body style='font-family:Microsoft YaHei;padding:24px;'>"
                                 "<h2>Protocol page failed to load</h2><p>%1</p><p>%2</p></body></html>")
                                 .arg(text, url);
        const QString dataUrl = QStringLiteral("data:text/html;charset=utf-8,%1")
                                    .arg(QString::fromUtf8(QUrl::toPercentEncoding(html)));
        frame->LoadURL(utf8StringFromQString(dataUrl));
        if (browser) {
            browser->GetHost()->WasResized();
            browser->GetHost()->Invalidate(PET_VIEW);
        }
    }

private:
    CefOffscreenBrowserWidget* widget_ = nullptr;
    std::string initialUrl_;
    int paintLogCount_ = 0;

    IMPLEMENT_REFCOUNTING(CefWidgetClient);
};

int executeCefSubProcess(HINSTANCE instance)
{
    CefMainArgs mainArgs(instance);
    return CefExecuteProcess(mainArgs, new QtLoginCefApp(), nullptr);
}

bool ensureCefInitialized()
{
    std::call_once(g_cefInitFlag, []() {
        logCef(L"ensureCefInitialized begin");
        CefEnableHighDPISupport();
        logCef(L"CefEnableHighDPISupport done");

        CefMainArgs mainArgs(GetModuleHandleW(nullptr));
        CefSettings settings;
        settings.no_sandbox = true;
        settings.windowless_rendering_enabled = true;
        settings.multi_threaded_message_loop = true;
        settings.log_severity = LOGSEVERITY_WARNING;
        const std::wstring logPath = executableDirectoryWide() + L"\\cef_debug.log";
        const std::wstring cachePath = cefCachePath();
        CefString(&settings.log_file).FromWString(logPath);
        CefString(&settings.cache_path).FromWString(cachePath);
        logCef(L"CefInitialize begin");

        g_cefInitialized = CefInitialize(mainArgs, settings, new QtLoginCefApp(), nullptr);
        logCef(g_cefInitialized ? L"CefInitialize succeeded" : L"CefInitialize failed");
    });
    return g_cefInitialized;
}

void shutdownCef()
{
    if (g_cefInitialized.exchange(false)) {
        CefShutdown();
    }
}

CefOffscreenBrowserWidget::CefOffscreenBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 260);
}

CefOffscreenBrowserWidget::~CefOffscreenBrowserWidget()
{
    if (client_) {
        client_->detach();
    }
    if (browser_) {
        browser_->GetHost()->CloseBrowser(true);
    }
}

void CefOffscreenBrowserWidget::loadUrl(const QString& url)
{
    pendingUrl_ = url.isEmpty() ? QStringLiteral("about:blank") : url;
    contentPainted_ = false;
    logCef(L"loadUrl " + wideStringFromQString(pendingUrl_));
    createBrowserIfNeeded();
    if (browser_) {
        loadUrlOnCefUi(browser_, utf8StringFromQString(pendingUrl_));
        invalidateView();
    }
}

void CefOffscreenBrowserWidget::loadFallbackHtml(const QString& title, const QString& url)
{
    if (contentPainted_) {
        return;
    }

    const QString urlHtml = url.toHtmlEscaped();
    const QString html = QStringLiteral(
        "<html><body style='margin:0;background:#fff9ef;color:#3d2515;font-family:Microsoft YaHei,Arial;padding:24px;'>"
        "<h2 style='margin-top:0;'>%1</h2>"
        "<p>协议页面远程加载较慢，当前先显示配置地址。CEF 离屏渲染已经启动，可以继续等待或复制地址到外部浏览器确认网络。</p>"
        "<p style='word-break:break-all;'><b>URL：</b><a href='%2'>%2</a></p>"
        "</body></html>")
        .arg(title.toHtmlEscaped(), urlHtml);
    const QString dataUrl = QStringLiteral("data:text/html;charset=utf-8,%1")
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(html)));

    logCef(L"loading fallback data page for " + wideStringFromQString(title));
    loadUrl(dataUrl);
}

void CefOffscreenBrowserWidget::sendMessageToWeb(const QString& name, const QString& payload)
{
    if (!browser_) {
        return;
    }

    CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(cefBridgeNativeToWebMessageName());
    CefRefPtr<CefListValue> list = message->GetArgumentList();
    list->SetString(0, utf8StringFromQString(name));
    list->SetString(1, utf8StringFromQString(payload));
    browser_->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
}

void CefOffscreenBrowserWidget::setWebMessageHandler(std::function<void(const QString&, const QString&)> handler)
{
    webMessageHandler_ = std::move(handler);
}

void CefOffscreenBrowserWidget::handleWebMessageFromCef(const std::string& name, const std::string& payload)
{
    logCef(L"web bridge message received");
    if (webMessageHandler_) {
        webMessageHandler_(qStringFromUtf8String(name), qStringFromUtf8String(payload));
    }
}

void CefOffscreenBrowserWidget::setBrowser(CefRefPtr<CefBrowser> browser)
{
    browser_ = browser;
    invalidateView();
}

void CefOffscreenBrowserWidget::updateImage(const QImage& image)
{
    image_ = image;
    const bool hadContent = contentPainted_;
    const int differenceCount = sampledPixelDifferenceCount(image_);
    contentPainted_ = contentPainted_ || differenceCount >= 20;
    if (!hadContent && contentPainted_) {
        logCef(L"content paint detected");
    } else if (!contentPainted_) {
        logCef(L"blank paint sample difference count=" + std::to_wstring(differenceCount));
    }
    update();
}

QSize CefOffscreenBrowserWidget::browserSize() const
{
    return size();
}

bool CefOffscreenBrowserWidget::hasContentPainted() const
{
    return contentPainted_;
}

void CefOffscreenBrowserWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    if (image_.isNull()) {
        const std::string background = protocolBrowserLoadingBackgroundColor();
        const std::string textColor = protocolBrowserLoadingTextColor();
        painter.fillRect(rect(), QColor(QString::fromLatin1(background.c_str(), static_cast<int>(background.size()))));
        painter.setPen(QColor(QString::fromLatin1(textColor.c_str(), static_cast<int>(textColor.size()))));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("Loading protocol page..."));
        return;
    }
    painter.drawImage(rect(), image_);
}

void CefOffscreenBrowserWidget::resizeEvent(QResizeEvent*)
{
    invalidateView();
}

void CefOffscreenBrowserWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (browser_) {
        browser_->GetHost()->SendMouseMoveEvent(toCefMouseEvent(event), false);
        event->accept();
    }
}

void CefOffscreenBrowserWidget::mousePressEvent(QMouseEvent* event)
{
    grabMouse();
    sendMouseClick(event, false);
    event->accept();
}

void CefOffscreenBrowserWidget::mouseReleaseEvent(QMouseEvent* event)
{
    sendMouseClick(event, true);
    if (event->buttons() == Qt::NoButton) {
        releaseMouse();
    }
    event->accept();
}

void CefOffscreenBrowserWidget::wheelEvent(QWheelEvent* event)
{
    if (!browser_) {
        return;
    }
    const QPoint delta = event->angleDelta();
    browser_->GetHost()->SendMouseWheelEvent(toCefMouseEvent(event), delta.x(), delta.y());
    event->accept();
}

void CefOffscreenBrowserWidget::createBrowserIfNeeded()
{
    if (browser_ || !ensureCefInitialized()) {
        return;
    }

    client_ = new CefWidgetClient(this, utf8StringFromQString(pendingUrl_));
    CefWindowInfo windowInfo;
    windowInfo.SetAsWindowless(nullptr);
    windowInfo.shared_texture_enabled = false;

    CefBrowserSettings browserSettings;
    const bool created = CefBrowserHost::CreateBrowser(windowInfo, client_, "about:blank", browserSettings, nullptr, nullptr);
    logCef(created ? L"CreateBrowser posted" : L"CreateBrowser failed");
}

void CefOffscreenBrowserWidget::sendMouseClick(QMouseEvent* event, bool mouseUp)
{
    if (!browser_) {
        return;
    }
    browser_->GetHost()->SendMouseClickEvent(toCefMouseEvent(event), toCefButton(event->button()), mouseUp, 1);
}

void CefOffscreenBrowserWidget::invalidateView()
{
    if (!browser_) {
        return;
    }
    browser_->GetHost()->WasHidden(false);
    browser_->GetHost()->SetFocus(true);
    browser_->GetHost()->WasResized();
    browser_->GetHost()->Invalidate(PET_VIEW);
}

}
