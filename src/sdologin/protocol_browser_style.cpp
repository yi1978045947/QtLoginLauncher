#include "protocol_browser_style.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace qtlogin::sdologin {

namespace {

std::string queryValue(const std::string& url, const std::string& key)
{
    const size_t queryPos = url.find('?');
    if (queryPos == std::string::npos) {
        return std::string();
    }

    const std::string prefix = key + "=";
    size_t pos = queryPos + 1;
    while (pos < url.size()) {
        const size_t end = url.find('&', pos);
        const std::string part = url.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        if (part.rfind(prefix, 0) == 0) {
            return part.substr(prefix.size());
        }
        if (end == std::string::npos) {
            break;
        }
        pos = end + 1;
    }
    return std::string();
}

std::string lowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string trimAscii(std::string value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
        value = value.substr(1, value.size() - 2);
    }
    return value;
}

void addUniqueUrl(std::vector<std::string>* urls, const std::string& url)
{
    if (url.empty()) {
        return;
    }
    if (std::find(urls->begin(), urls->end(), url) == urls->end()) {
        urls->push_back(url);
    }
}

void addProtocolImageUrl(std::vector<std::string>* urls, const std::string& baseUrl, std::string rawUrl)
{
    rawUrl = trimAscii(rawUrl);
    const std::string lowerRaw = lowerAscii(rawUrl);
    if (rawUrl.empty() ||
        lowerRaw.rfind("data:", 0) == 0 ||
        lowerRaw.rfind("javascript:", 0) == 0) {
        return;
    }
    addUniqueUrl(urls, resolveProtocolResourceUrl(baseUrl, rawUrl));
}

}

std::string protocolBrowserDialogStyleSheet()
{
    return
        "QDialog{background:#ffffff;}"
        "QLabel{color:#3d2515;font:12px 'Microsoft YaHei';}"
        "QTextBrowser{background:#ffffff;color:#3d2515;border:1px solid #e7dfd6;font:14px 'Microsoft YaHei';}"
        "QPushButton{background:#a65f24;color:#fff;border:0;font:16px 'Microsoft YaHei';}"
        "QPushButton:hover{background:#bd7130;}";
}

std::string protocolBrowserLoadingBackgroundColor()
{
    return "#ffffff";
}

std::string protocolBrowserLoadingTextColor()
{
    return "#3d2515";
}

std::string protocolBrowserQtFallbackStatusText()
{
    return "Qt network protocol rendering";
}

bool shouldLoadProtocolUrlWithQtFallback(const std::string& url)
{
    return url.rfind("https://", 0) == 0 || url.rfind("http://", 0) == 0;
}

bool shouldLoadSdoAgreementContentJsonp(const std::string& url)
{
    return url.find("we.sdoprofile.com/common/static/register/public/user_agreements.html") != std::string::npos;
}

std::string sdoAgreementContentJsonpUrl(const std::string& url)
{
    std::string appId = queryValue(url, "appId");
    if (appId.empty()) {
        appId = "undefined";
    }
    std::string result = "https://utility.sdo.com/agreement/content?appId=" + appId;
    const std::string scene = queryValue(url, "scene");
    if (!scene.empty()) {
        result += "&scene=" + scene;
    }
    result += "&fn_callback=callback";
    return result;
}

bool shouldLoadSdoViolationNewsContent(const std::string& url)
{
    return url.find("mxd.web.sdo.com/web7/news/violation.html") != std::string::npos;
}

std::string sdoViolationNewsContentUrl(const std::string&)
{
    return "https://mxd.web.sdo.com/web7/Handler/NewsContent.ashx?id=350555";
}

std::string resolveProtocolResourceUrl(const std::string& baseUrl, const std::string& resourceUrl)
{
    if (resourceUrl.rfind("https://", 0) == 0 || resourceUrl.rfind("http://", 0) == 0) {
        return resourceUrl;
    }
    if (resourceUrl.rfind("//", 0) == 0) {
        const size_t schemeEnd = baseUrl.find(':');
        const std::string scheme = schemeEnd == std::string::npos ? "https" : baseUrl.substr(0, schemeEnd);
        return scheme + ":" + resourceUrl;
    }

    const size_t schemeEnd = baseUrl.find("://");
    if (schemeEnd == std::string::npos) {
        return resourceUrl;
    }
    const size_t hostBegin = schemeEnd + 3;
    const size_t pathBegin = baseUrl.find('/', hostBegin);
    const std::string origin = pathBegin == std::string::npos ? baseUrl : baseUrl.substr(0, pathBegin);
    if (resourceUrl.empty() || resourceUrl[0] == '#') {
        return baseUrl + resourceUrl;
    }
    if (resourceUrl[0] == '/') {
        return origin + resourceUrl;
    }
    const size_t queryBegin = baseUrl.find('?', hostBegin);
    const size_t effectiveEnd = queryBegin == std::string::npos ? baseUrl.size() : queryBegin;
    const size_t dirEnd = baseUrl.rfind('/', effectiveEnd);
    if (dirEnd == std::string::npos || dirEnd < hostBegin) {
        return origin + "/" + resourceUrl;
    }
    return baseUrl.substr(0, dirEnd + 1) + resourceUrl;
}

std::vector<std::string> protocolImageResourceUrls(const std::string& baseUrl, const std::string& html)
{
    std::vector<std::string> urls;
    const std::string lower = lowerAscii(html);

    size_t pos = 0;
    while ((pos = lower.find("src", pos)) != std::string::npos) {
        size_t cursor = pos + 3;
        while (cursor < lower.size() && std::isspace(static_cast<unsigned char>(lower[cursor]))) {
            ++cursor;
        }
        if (cursor >= lower.size() || lower[cursor] != '=') {
            pos = cursor;
            continue;
        }
        ++cursor;
        while (cursor < lower.size() && std::isspace(static_cast<unsigned char>(lower[cursor]))) {
            ++cursor;
        }
        if (cursor >= html.size()) {
            break;
        }
        const char quote = html[cursor];
        size_t valueBegin = cursor;
        size_t valueEnd = cursor;
        if (quote == '"' || quote == '\'') {
            valueBegin = cursor + 1;
            valueEnd = html.find(quote, valueBegin);
            if (valueEnd == std::string::npos) {
                break;
            }
        } else {
            while (valueEnd < html.size() &&
                   !std::isspace(static_cast<unsigned char>(html[valueEnd])) &&
                   html[valueEnd] != '>') {
                ++valueEnd;
            }
        }
        addProtocolImageUrl(&urls, baseUrl, html.substr(valueBegin, valueEnd - valueBegin));
        pos = valueEnd + 1;
    }

    pos = 0;
    const std::string backgroundNeedle = "background-image";
    while ((pos = lower.find(backgroundNeedle, pos)) != std::string::npos) {
        const size_t urlPos = lower.find("url(", pos + backgroundNeedle.size());
        if (urlPos == std::string::npos) {
            break;
        }
        const size_t valueBegin = urlPos + 4;
        const size_t valueEnd = html.find(')', valueBegin);
        if (valueEnd == std::string::npos) {
            break;
        }
        addProtocolImageUrl(&urls, baseUrl, html.substr(valueBegin, valueEnd - valueBegin));
        pos = valueEnd + 1;
    }

    return urls;
}

bool shouldOpenProtocolLinkExternally(const std::string& url)
{
    return url.rfind("https://", 0) == 0 || url.rfind("http://", 0) == 0;
}

std::string protocolExternalLinkTarget(const std::string& baseUrl, const std::string& linkUrl)
{
    const std::string trimmedLink = trimAscii(linkUrl);
    const std::string lowerLink = lowerAscii(trimmedLink);
    if (trimmedLink.empty() ||
        trimmedLink[0] == '#' ||
        lowerLink.rfind("javascript:", 0) == 0) {
        return std::string();
    }

    const std::string resolvedUrl = resolveProtocolResourceUrl(baseUrl, trimmedLink);
    if (!shouldOpenProtocolLinkExternally(resolvedUrl)) {
        return std::string();
    }
    return resolvedUrl;
}

std::string rewriteProtocolResourceUrls(
    const std::string& html,
    const std::vector<std::pair<std::string, std::string>>& replacements)
{
    std::string result = html;
    for (const auto& replacement : replacements) {
        const std::string& from = replacement.first;
        const std::string& to = replacement.second;
        if (from.empty()) {
            continue;
        }
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.size(), to);
            pos += to.size();
        }
    }
    return result;
}

bool shouldShowProtocolLoadError(int cefErrorCode)
{
    constexpr int kCefErrAborted = -3;
    return cefErrorCode != kCefErrAborted;
}

}
