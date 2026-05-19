#include "protocol_browser_dialog.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <QByteArray>
#include <QDesktopServices>
#include <QFont>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#endif

#ifdef QTLOGIN_ENABLE_CEF_OSR
#include "cef_offscreen_browser.h"
#endif
#include "dpi_scaler.h"
#include "protocol_acceptance_model.h"
#include "protocol_browser_style.h"

#include <windows.h>
#include <winhttp.h>

namespace qtlogin::sdologin {

namespace {

constexpr size_t kMaxProtocolHtmlBytes = 8u * 1024u * 1024u;

struct WinHttpHandle {
    WinHttpHandle() = default;
    explicit WinHttpHandle(HINTERNET handle) : handle(handle) {}
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    ~WinHttpHandle()
    {
        if (handle) {
            WinHttpCloseHandle(handle);
        }
    }

    HINTERNET get() const { return handle; }

    HINTERNET handle = nullptr;
};

struct ProtocolFetchResult {
    bool ok = false;
    DWORD statusCode = 0;
    std::wstring contentType;
    std::wstring error;
    std::string body;
};

struct PrefetchedProtocolImage {
    QString url;
    QImage image;
};

ProtocolFetchResult fetchProtocolHtml(const std::wstring& url);
void protocolDialogTraceLine(const std::wstring& text);

class ProtocolTextBrowser final : public QTextBrowser {
public:
    ProtocolTextBrowser(const QString& pageUrl, QWidget* parent)
        : QTextBrowser(parent),
          pageUrl_(pageUrl)
    {
        setOpenLinks(false);
        setOpenExternalLinks(false);
        document()->setBaseUrl(QUrl(pageUrl_));
        connect(this, &QTextBrowser::anchorClicked, this, [this](const QUrl& url) {
            const QUrl absoluteUrl = url.isRelative() ? QUrl(pageUrl_).resolved(url) : url;
            const std::string link = absoluteUrl.toString().toStdString();
            if (shouldOpenProtocolLinkExternally(link)) {
                QDesktopServices::openUrl(absoluteUrl);
                return;
            }
            if (absoluteUrl.hasFragment()) {
                scrollToAnchor(absoluteUrl.fragment());
            }
        });
    }

private:
    QString pageUrl_;
};

std::wstring protocolDialogLogPath()
{
    std::vector<wchar_t> buffer(MAX_PATH);
    const DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size == 0) {
        return L"qtlogin_rewrite.log";
    }
    std::wstring path(buffer.data(), size);
    const size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        path.resize(pos + 1);
    } else {
        path.clear();
    }
    path += L"qtlogin_rewrite.log";
    return path;
}

void protocolDialogTraceLine(const std::wstring& text)
{
    std::wstring line = L"[protocol_dialog] ";
    line += text;
    line += L"\r\n";
    OutputDebugStringW(line.c_str());
    std::wofstream file(protocolDialogLogPath(), std::ios::app);
    if (file.is_open()) {
        file << line;
    }
}

void protocolDialogTrace(const wchar_t* text)
{
    protocolDialogTraceLine(text ? std::wstring(text) : std::wstring());
}

std::wstring formatWindowsError(DWORD error)
{
    wchar_t* message = nullptr;
    const DWORD count = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        0,
        reinterpret_cast<LPWSTR>(&message),
        0,
        nullptr);
    std::wstring result;
    if (count > 0 && message) {
        result.assign(message, count);
        while (!result.empty() && (result.back() == L'\r' || result.back() == L'\n' || result.back() == L'.')) {
            result.pop_back();
        }
    }
    if (message) {
        LocalFree(message);
    }
    if (result.empty()) {
        result = L"WinHTTP error " + std::to_wstring(error);
    }
    return result;
}

std::wstring fromQString(const QString& value)
{
    return std::wstring(reinterpret_cast<const wchar_t*>(value.utf16()), static_cast<size_t>(value.size()));
}

QString fromWide(const std::wstring& value)
{
    return QString::fromWCharArray(value.c_str(), static_cast<int>(value.size()));
}

std::wstring queryResponseHeader(HINTERNET request, DWORD header)
{
    DWORD bytes = 0;
    if (WinHttpQueryHeaders(request, header, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &bytes, WINHTTP_NO_HEADER_INDEX)) {
        return std::wstring();
    }
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes == 0) {
        return std::wstring();
    }

    std::vector<wchar_t> buffer(bytes / sizeof(wchar_t) + 1, L'\0');
    if (!WinHttpQueryHeaders(request, header, WINHTTP_HEADER_NAME_BY_INDEX, buffer.data(), &bytes, WINHTTP_NO_HEADER_INDEX)) {
        return std::wstring();
    }
    return std::wstring(buffer.data());
}

ProtocolFetchResult fetchProtocolHtml(const std::wstring& url)
{
    ProtocolFetchResult result;

    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &components)) {
        result.error = L"Cannot parse protocol URL: " + formatWindowsError(GetLastError());
        return result;
    }

    const std::wstring host(components.lpszHostName, components.dwHostNameLength);
    std::wstring path = components.dwUrlPathLength > 0
        ? std::wstring(components.lpszUrlPath, components.dwUrlPathLength)
        : std::wstring(L"/");
    if (components.dwExtraInfoLength > 0) {
        path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
    }

    WinHttpHandle session(WinHttpOpen(
        L"QtLoginRewrite/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0));
    if (!session.get()) {
        result.error = L"WinHttpOpen failed: " + formatWindowsError(GetLastError());
        return result;
    }

    WinHttpSetTimeouts(session.get(), 5000, 5000, 10000, 15000);
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
    DWORD secureProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    WinHttpSetOption(session.get(), WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols, sizeof(secureProtocols));
#endif

    WinHttpHandle connection(WinHttpConnect(session.get(), host.c_str(), components.nPort, 0));
    if (!connection.get()) {
        result.error = L"WinHttpConnect failed: " + formatWindowsError(GetLastError());
        return result;
    }

    const wchar_t* acceptTypes[] = {
        L"text/html",
        L"application/xhtml+xml",
        L"*/*",
        nullptr,
    };
    const DWORD requestFlags = components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    WinHttpHandle request(WinHttpOpenRequest(
        connection.get(),
        L"GET",
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        acceptTypes,
        requestFlags));
    if (!request.get()) {
        result.error = L"WinHttpOpenRequest failed: " + formatWindowsError(GetLastError());
        return result;
    }

    DWORD redirects = 5;
    WinHttpSetOption(request.get(), WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS, &redirects, sizeof(redirects));

    const wchar_t extraHeaders[] =
        L"Accept-Language: zh-CN,zh;q=0.9,en;q=0.6\r\n"
        L"Cache-Control: no-cache\r\n";
    if (!WinHttpSendRequest(
            request.get(),
            extraHeaders,
            static_cast<DWORD>(-1),
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0)) {
        result.error = L"WinHttpSendRequest failed: " + formatWindowsError(GetLastError());
        return result;
    }

    if (!WinHttpReceiveResponse(request.get(), nullptr)) {
        result.error = L"WinHttpReceiveResponse failed: " + formatWindowsError(GetLastError());
        return result;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (WinHttpQueryHeaders(
            request.get(),
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusSize,
            WINHTTP_NO_HEADER_INDEX)) {
        result.statusCode = statusCode;
    }
    result.contentType = queryResponseHeader(request.get(), WINHTTP_QUERY_CONTENT_TYPE);

    if (statusCode < 200 || statusCode >= 300) {
        result.error = L"Protocol server returned HTTP " + std::to_wstring(statusCode);
        return result;
    }

    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.get(), &available)) {
            result.error = L"WinHttpQueryDataAvailable failed: " + formatWindowsError(GetLastError());
            return result;
        }
        if (available == 0) {
            break;
        }
        if (result.body.size() + available > kMaxProtocolHtmlBytes) {
            result.error = L"Protocol page is larger than the local fallback limit.";
            return result;
        }

        std::string chunk;
        chunk.resize(available);
        DWORD read = 0;
        if (!WinHttpReadData(request.get(), chunk.data(), available, &read)) {
            result.error = L"WinHttpReadData failed: " + formatWindowsError(GetLastError());
            return result;
        }
        chunk.resize(read);
        result.body.append(chunk);
    }

    if (result.body.empty()) {
        result.error = L"Protocol page returned an empty response.";
        return result;
    }

    result.ok = true;
    return result;
}

QString extractCharset(const QString& contentType)
{
    const QString lower = contentType.toLower();
    const int pos = lower.indexOf(QStringLiteral("charset="));
    if (pos < 0) {
        return QString();
    }

    const int start = pos + 8;
    int end = contentType.indexOf(QLatin1Char(';'), start);
    if (end < 0) {
        end = contentType.size();
    }
    QString charset = contentType.mid(start, end - start).trimmed();
    if ((charset.startsWith(QLatin1Char('"')) && charset.endsWith(QLatin1Char('"'))) ||
        (charset.startsWith(QLatin1Char('\'')) && charset.endsWith(QLatin1Char('\'')))) {
        charset = charset.mid(1, charset.size() - 2);
    }
    return charset;
}

QString decodeProtocolHtml(const ProtocolFetchResult& result)
{
    const QByteArray bytes(result.body.data(), static_cast<int>(result.body.size()));
    const QString contentType = fromWide(result.contentType);
    const QString charset = extractCharset(contentType);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (!charset.isEmpty()) {
        QTextCodec* codec = QTextCodec::codecForName(charset.toLatin1());
        if (codec) {
            return codec->toUnicode(bytes);
        }
    }
#endif
    return QString::fromUtf8(bytes);
}

QString normalizedProtocolHtmlForTextBrowser(QString html)
{
    html.replace(QRegularExpression(QStringLiteral("display\\s*:\\s*none\\s*;?"), QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("display:block;"));
    html.replace(QStringLiteral("$(\"body\").show();"), QString());
    html.replace(QStringLiteral("$('body').show();"), QString());
    const QRegularExpression backgroundImageRegex(
        QStringLiteral("\\.inscribe-img[\\s\\S]*?background-image\\s*:\\s*url\\(([^\\)]+)\\)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch backgroundMatch = backgroundImageRegex.match(html);
    if (backgroundMatch.hasMatch()) {
        QString imageUrl = backgroundMatch.captured(1).trimmed();
        if ((imageUrl.startsWith(QLatin1Char('\'')) && imageUrl.endsWith(QLatin1Char('\''))) ||
            (imageUrl.startsWith(QLatin1Char('"')) && imageUrl.endsWith(QLatin1Char('"')))) {
            imageUrl = imageUrl.mid(1, imageUrl.size() - 2);
        }
        const QString imageHtml = QStringLiteral(
            "<p style=\"text-align:center;\"><img src=\"%1\" width=\"250\" /></p>")
            .arg(imageUrl.toHtmlEscaped());
        html.replace(
            QRegularExpression(
                QStringLiteral("<div([^>]*)class=[\"'][^\"']*inscribe-img[^\"']*[\"'][^>]*>\\s*</div>"),
                QRegularExpression::CaseInsensitiveOption),
            imageHtml);
    }
    return html;
}

QString sdoAgreementHtmlFromJsonp(const QString& jsonp)
{
    const int open = jsonp.indexOf(QLatin1Char('('));
    const int close = jsonp.lastIndexOf(QLatin1Char(')'));
    if (open < 0 || close <= open) {
        return QString();
    }

    const QByteArray json = jsonp.mid(open + 1, close - open - 1).toUtf8();
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return QString();
    }

    const QJsonObject root = document.object();
    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    const QString agreement = data.value(QStringLiteral("servicerAgreement")).toString();
    if (agreement.trimmed().isEmpty()) {
        return QString();
    }

    QString body;
    const QStringList lines = agreement.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            body += QStringLiteral("<br/>");
            continue;
        }
        body += QStringLiteral("<p>%1</p>").arg(trimmed.toHtmlEscaped());
    }

    return QStringLiteral(
               "<html><body style=\"font-family:'Microsoft YaHei';color:#222;background:#ffffff;"
               "font-size:14px;line-height:1.8;padding:24px 36px;\">"
               "<h1 style=\"text-align:center;font-size:24px;\">%1</h1>"
               "%2"
               "</body></html>")
        .arg(QString::fromUtf8(u8"用户协议").toHtmlEscaped(), body);
}

QString sdoViolationNewsHtmlFromJson(const QString& json, const QString& fallbackTitle)
{
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return QString();
    }

    const QJsonObject root = document.object();
    const QJsonObject data = root.value(QStringLiteral("data")).toObject();
    const QString content = data.value(QStringLiteral("Content")).toString().trimmed();
    if (content.isEmpty()) {
        return QString();
    }

    QString title = data.value(QStringLiteral("Title")).toString().trimmed();
    if (title.isEmpty()) {
        title = fallbackTitle;
    }
    const QString publishDate = data.value(QStringLiteral("PublishDate")).toString().trimmed();
    const QString dateHtml = publishDate.isEmpty()
        ? QString()
        : QStringLiteral("<div class=\"publish\">%1</div>").arg(publishDate.toHtmlEscaped());

    return QStringLiteral(
               "<html><head><meta charset=\"utf-8\"><style>"
               "body{font-family:'Microsoft YaHei';color:#222;background:#fff;"
               "font-size:14px;line-height:1.75;padding:24px 36px;}"
               "h1{font-size:22px;line-height:1.35;text-align:center;margin:0 0 12px 0;}"
               ".publish{text-align:center;color:#666;margin:0 0 22px 0;}"
               "p{margin:0 0 12px 0;}"
               "a{color:#1b66c9;}"
               "</style></head><body><h1>%1</h1>%2<div>%3</div></body></html>")
        .arg(title.toHtmlEscaped(), dateHtml, content);
}

QString protocolStatusText()
{
    return QStringLiteral("CEF 109 off-screen rendering");
}

bool shouldUseCefProtocolBrowser()
{
    return true;
}

QString makeFallbackLoadingHtml(const QString& title, const QString& url)
{
    const QString link = url.isEmpty()
        ? QString()
        : QStringLiteral("<p><a href=\"%1\">%1</a></p>").arg(url.toHtmlEscaped());
    return QStringLiteral(
               "<html><body style=\"font-family:'Microsoft YaHei';color:#3d2515;background:#ffffff;\">"
               "<h2>%1</h2>"
               "<p>%2</p>"
               "%3"
               "</body></html>")
        .arg(title.toHtmlEscaped(),
             QString::fromUtf8(u8"正在加载协议内容，请稍候...").toHtmlEscaped(),
             link);
}

QString makeFallbackErrorHtml(const QString& title, const QString& url, const QString& error)
{
    const QString link = url.isEmpty()
        ? QString()
        : QStringLiteral("<p>%1 <a href=\"%2\">%2</a></p>")
              .arg(QString::fromUtf8(u8"配置 URL：").toHtmlEscaped(), url.toHtmlEscaped());
    return QStringLiteral(
               "<html><body style=\"font-family:'Microsoft YaHei';color:#3d2515;background:#ffffff;\">"
               "<h2>%1</h2>"
               "<p><b>%2</b></p>"
               "<p>%3</p>"
               "%4"
               "</body></html>")
        .arg(title.toHtmlEscaped(),
             QString::fromUtf8(u8"协议内容加载失败").toHtmlEscaped(),
             error.toHtmlEscaped(),
             link);
}

std::vector<PrefetchedProtocolImage> prefetchProtocolImages(const QString& baseUrl, const QString& html)
{
    std::vector<PrefetchedProtocolImage> images;
    // Keep the Qt fallback stable: injecting remote images into QTextDocument
    // can crash while rendering complex privacy-policy HTML. Full image support
    // should move back to the CEF/real-webview path.
    (void)baseUrl;
    (void)html;
    return images;

#if 0
    const std::vector<std::string> urls =
        protocolImageResourceUrls(baseUrl.toStdString(), html.toUtf8().constData());
    for (const std::string& imageUrl : urls) {
        if (!shouldLoadProtocolUrlWithQtFallback(imageUrl)) {
            continue;
        }
        const std::wstring requestUrl(imageUrl.begin(), imageUrl.end());
        ProtocolFetchResult imageResult = fetchProtocolHtml(requestUrl);
        protocolDialogTraceLine(
            L"prefetch image done ok=" + std::to_wstring(imageResult.ok ? 1 : 0) +
            L" status=" + std::to_wstring(imageResult.statusCode) +
            L" bytes=" + std::to_wstring(imageResult.body.size()) +
            L" urlLen=" + std::to_wstring(requestUrl.size()));
        if (!imageResult.ok || imageResult.body.empty()) {
            continue;
        }
        QImage decodedImage;
        if (!decodedImage.loadFromData(
                reinterpret_cast<const uchar*>(imageResult.body.data()),
                static_cast<int>(imageResult.body.size()))) {
            protocolDialogTraceLine(L"prefetch image decode failed urlLen=" + std::to_wstring(requestUrl.size()));
            continue;
        }
        PrefetchedProtocolImage image;
        image.url = QString::fromStdString(imageUrl);
        image.image = decodedImage;
        images.push_back(image);
    }
    return images;
#endif
}

}

ProtocolBrowserDialog::ProtocolBrowserDialog(const QString& title, const QString& url, QWidget* parent)
    : ProtocolBrowserDialog(title, url, 0.0, parent)
{
}

ProtocolBrowserDialog::ProtocolBrowserDialog(const QString& title, const QString& url, double requestedDpiScale, QWidget* parent)
    : ProtocolBrowserDialog(title, url, requestedDpiScale, 0, parent)
{
}

ProtocolBrowserDialog::ProtocolBrowserDialog(
    const QString& title,
    const QString& url,
    double requestedDpiScale,
    int agreeDelaySeconds,
    QWidget* parent)
    : QDialog(parent)
{
    protocolDialogTrace(L"ctor begin");
    const int systemDpi = currentSystemDpiForLegacyScaling();
    const double environmentScale = legacyDpiScaleFactorFromEnvironment();
    const double legacyDpiScaleFactor = chooseLegacyDpiScaleFactor(
        requestedDpiScale,
        environmentScale,
        systemDpi);
    {
        std::wostringstream out;
        out << L"dpi computed scale=" << legacyDpiScaleFactor
            << L" requested=" << requestedDpiScale
            << L" envScale=" << environmentScale
            << L" systemDpi=" << systemDpi;
        protocolDialogTraceLine(out.str());
    }
    const auto scalePx = [legacyDpiScaleFactor](int value) {
        return scaleLegacyPixel(value, legacyDpiScaleFactor);
    };

    setWindowTitle(title);
    protocolDialogTrace(L"title set");
    setFixedSize(scalePx(616), scalePx(530));
    protocolDialogTrace(L"fixed size set");
    setModal(true);
    protocolDialogTrace(L"modal set");
    QFont scaledFont = font();
    protocolDialogTrace(L"font read");
    scaledFont.setPixelSize(scalePx(12));
    setFont(scaledFont);
    protocolDialogTrace(L"font set");
    const std::string styleSheet = scaleLegacyStyleSheetPixels(
        protocolBrowserDialogStyleSheet(),
        legacyDpiScaleFactor);
    setStyleSheet(QString::fromLatin1(styleSheet.c_str(), static_cast<int>(styleSheet.size())));
    protocolDialogTrace(L"base dialog configured");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(scalePx(24), scalePx(18), scalePx(24), scalePx(18));
    layout->setSpacing(scalePx(10));

    auto* status = new QLabel(protocolStatusText(), this);
    status->setObjectName(QStringLiteral("protocolStatus"));
    status->setWordWrap(true);
    layout->addWidget(status);
    protocolDialogTrace(L"status label added");

#ifdef QTLOGIN_ENABLE_CEF_OSR
    if (shouldUseCefProtocolBrowser()) {
        protocolDialogTrace(L"cef widget create begin");
        auto* browser = new CefOffscreenBrowserWidget(this);
        browser->setWebMessageHandler([browser](const QString& name, const QString& payload) {
            protocolDialogTraceLine(
                L"cef web message nameLen=" + std::to_wstring(name.size()) +
                L" payloadLen=" + std::to_wstring(payload.size()));
            browser->sendMessageToWeb(QStringLiteral("nativeAck"), name);
        });
        protocolDialogTrace(L"cef widget created");
        layout->addWidget(browser, 1);
        QTimer::singleShot(0, browser, [browser, url]() {
            browser->loadUrl(url);
        });
        QTimer::singleShot(800, browser, [browser]() {
            browser->sendMessageToWeb(QStringLiteral("nativeReady"), QStringLiteral("{}"));
        });
        QPointer<CefOffscreenBrowserWidget> cefBrowser(browser);
        QTimer::singleShot(15000, this, [cefBrowser]() {
            if (!cefBrowser || cefBrowser->hasContentPainted()) {
                return;
            }
            protocolDialogTrace(L"cef still waiting for first content paint");
        });
    } else {
        auto* browser = new ProtocolTextBrowser(url, this);
        browser->setHtml(fallbackLoadingHtml(title, url));
        layout->addWidget(browser, 1);
        protocolDialogTrace(L"text fallback browser added");
        loadFallbackUrl(browser, status, title, url);
    }
#else
    auto* browser = new ProtocolTextBrowser(url, this);
    browser->setHtml(fallbackLoadingHtml(title, url));
    layout->addWidget(browser, 1);
    protocolDialogTrace(L"text fallback browser added");
    loadFallbackUrl(browser, status, title, url);
#endif

    auto* ok = new QPushButton(QString::fromUtf8(u8"同意并返回"), this);
    ok->setFixedHeight(scalePx(42));
    layout->addWidget(ok);
    const int normalizedAgreeDelay = normalizedProtocolAgreeDelaySeconds(agreeDelaySeconds);
    if (agreeDelaySeconds > 0) {
        ok->setEnabled(false);
        ok->setText(QStringLiteral("%1秒").arg(normalizedAgreeDelay));
        auto* agreeTimer = new QTimer(ok);
        int remainingSeconds = normalizedAgreeDelay;
        connect(agreeTimer, &QTimer::timeout, ok, [ok, agreeTimer, remainingSeconds]() mutable {
            --remainingSeconds;
            if (remainingSeconds <= 0) {
                agreeTimer->stop();
                ok->setEnabled(true);
                ok->setText(QString::fromUtf8(u8"同意并返回"));
                return;
            }
            ok->setText(QStringLiteral("%1秒").arg(remainingSeconds));
        });
        agreeTimer->start(1000);
    }
    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    protocolDialogTrace(L"ctor end");
}

QString ProtocolBrowserDialog::fallbackLoadingHtml(const QString& title, const QString& url) const
{
    return makeFallbackLoadingHtml(title, url);
}

QString ProtocolBrowserDialog::fallbackErrorHtml(const QString& title, const QString& url, const QString& error) const
{
    return makeFallbackErrorHtml(title, url, error);
}

void ProtocolBrowserDialog::loadFallbackUrl(QTextBrowser* browser, QLabel* status, const QString& title, const QString& url)
{
    const QByteArray urlUtf8 = url.toUtf8();
    if (!shouldLoadProtocolUrlWithQtFallback(urlUtf8.constData())) {
        protocolDialogTrace(L"text fallback skipped unsupported url");
        browser->setHtml(fallbackErrorHtml(title, url, QString::fromUtf8(u8"当前协议地址为空或不是 HTTP/HTTPS 地址。")));
        return;
    }

    protocolDialogTraceLine(L"text fallback request begin urlLen=" + std::to_wstring(url.size()));
    QPointer<QTextBrowser> browserPtr(browser);
    QPointer<QLabel> statusPtr(status);
    const std::wstring requestUrl = fromQString(url);
    const QString requestTitle = title;
    const QString requestUrlText = url;

    std::thread([browserPtr, statusPtr, requestUrl, requestTitle, requestUrlText]() mutable {
        ProtocolFetchResult result = fetchProtocolHtml(requestUrl);
        protocolDialogTraceLine(
            L"text fallback request done ok=" + std::to_wstring(result.ok ? 1 : 0) +
            L" status=" + std::to_wstring(result.statusCode) +
            L" bytes=" + std::to_wstring(result.body.size()));
        QString htmlToShow;
        if (result.ok) {
            const QByteArray requestUrlUtf8 = requestUrlText.toUtf8();
            if (shouldLoadSdoAgreementContentJsonp(requestUrlUtf8.constData())) {
                const std::string jsonpUrl = sdoAgreementContentJsonpUrl(requestUrlUtf8.constData());
                const std::wstring jsonpRequestUrl(jsonpUrl.begin(), jsonpUrl.end());
                ProtocolFetchResult jsonpResult = fetchProtocolHtml(jsonpRequestUrl);
                protocolDialogTraceLine(
                    L"sdo agreement jsonp request done ok=" + std::to_wstring(jsonpResult.ok ? 1 : 0) +
                    L" status=" + std::to_wstring(jsonpResult.statusCode) +
                    L" bytes=" + std::to_wstring(jsonpResult.body.size()));
                if (jsonpResult.ok) {
                    htmlToShow = sdoAgreementHtmlFromJsonp(decodeProtocolHtml(jsonpResult));
                }
            } else if (shouldLoadSdoViolationNewsContent(requestUrlUtf8.constData())) {
                const std::string newsUrl = sdoViolationNewsContentUrl(requestUrlUtf8.constData());
                const std::wstring newsRequestUrl(newsUrl.begin(), newsUrl.end());
                ProtocolFetchResult newsResult = fetchProtocolHtml(newsRequestUrl);
                protocolDialogTraceLine(
                    L"sdo violation news request done ok=" + std::to_wstring(newsResult.ok ? 1 : 0) +
                    L" status=" + std::to_wstring(newsResult.statusCode) +
                    L" bytes=" + std::to_wstring(newsResult.body.size()));
                if (newsResult.ok) {
                    htmlToShow = sdoViolationNewsHtmlFromJson(decodeProtocolHtml(newsResult), requestTitle);
                }
            }
            if (htmlToShow.isEmpty()) {
                htmlToShow = normalizedProtocolHtmlForTextBrowser(decodeProtocolHtml(result));
            }
        }
        std::vector<PrefetchedProtocolImage> prefetchedImages;
        if (result.ok && !htmlToShow.isEmpty()) {
            prefetchedImages = prefetchProtocolImages(requestUrlText, htmlToShow);
        }

        if (!browserPtr) {
            return;
        }

        QMetaObject::invokeMethod(
            browserPtr.data(),
            [browserPtr, statusPtr, requestTitle, requestUrlText, result, htmlToShow, prefetchedImages]() mutable {
                if (!browserPtr) {
                    return;
                }
                if (result.ok) {
                    if (statusPtr) {
                        statusPtr->hide();
                    }
                    browserPtr->document()->setBaseUrl(QUrl(requestUrlText));
                    for (const PrefetchedProtocolImage& image : prefetchedImages) {
                        browserPtr->document()->addResource(
                            QTextDocument::ImageResource,
                            QUrl(image.url),
                            image.image);
                    }
                    browserPtr->setHtml(htmlToShow);
                    return;
                }

                if (statusPtr) {
                    statusPtr->setText(QString::fromUtf8(u8"协议内容加载失败"));
                }
                browserPtr->setHtml(makeFallbackErrorHtml(
                    requestTitle,
                    requestUrlText,
                    fromWide(result.error)));
            },
            Qt::QueuedConnection);
    }).detach();
}

}
