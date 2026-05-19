#pragma once

#include <string>
#include <vector>

namespace qtlogin::sdologin {

struct ProtocolBrowserLaunchOptions {
    std::wstring title;
    std::wstring url;
    void* ownerHwnd = nullptr;
    double legacyDpiScaleFactor = 0.0;
    int agreeDelaySeconds = 0;
    bool prewarmOnly = false;
};

std::vector<std::wstring> buildProtocolBrowserArguments(const ProtocolBrowserLaunchOptions& options);
std::vector<std::wstring> buildProtocolBrowserArguments(const std::wstring& title, const std::wstring& url);
std::wstring protocolBrowserExecutablePath(const std::wstring& executableDirectory);
bool launchProtocolBrowser(const ProtocolBrowserLaunchOptions& options);
bool launchProtocolBrowser(const std::wstring& title, const std::wstring& url);

}
