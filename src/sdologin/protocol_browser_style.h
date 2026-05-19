#pragma once

#include <string>
#include <utility>
#include <vector>

namespace qtlogin::sdologin {

std::string protocolBrowserDialogStyleSheet();
std::string protocolBrowserLoadingBackgroundColor();
std::string protocolBrowserLoadingTextColor();
std::string protocolBrowserQtFallbackStatusText();
bool shouldLoadProtocolUrlWithQtFallback(const std::string& url);
bool shouldLoadSdoAgreementContentJsonp(const std::string& url);
std::string sdoAgreementContentJsonpUrl(const std::string& url);
bool shouldLoadSdoViolationNewsContent(const std::string& url);
std::string sdoViolationNewsContentUrl(const std::string& url);
std::string resolveProtocolResourceUrl(const std::string& baseUrl, const std::string& resourceUrl);
std::vector<std::string> protocolImageResourceUrls(const std::string& baseUrl, const std::string& html);
bool shouldOpenProtocolLinkExternally(const std::string& url);
std::string protocolExternalLinkTarget(const std::string& baseUrl, const std::string& linkUrl);
std::string rewriteProtocolResourceUrls(
    const std::string& html,
    const std::vector<std::pair<std::string, std::string>>& replacements);
bool shouldShowProtocolLoadError(int cefErrorCode);

}
