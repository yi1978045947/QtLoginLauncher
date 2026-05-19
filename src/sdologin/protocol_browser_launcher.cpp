#include "protocol_browser_launcher.h"

#include <cstdint>

#include <windows.h>

#include "logger.h"
#include "process_util.h"

namespace qtlogin::sdologin {

std::vector<std::wstring> buildProtocolBrowserArguments(const ProtocolBrowserLaunchOptions& options)
{
    std::vector<std::wstring> args = {
        L"--title=" + options.title,
        L"--url=" + options.url,
    };
    if (options.ownerHwnd) {
        args.push_back(L"--owner-hwnd=" + std::to_wstring(reinterpret_cast<uintptr_t>(options.ownerHwnd)));
    }
    if (options.legacyDpiScaleFactor > 0.0) {
        wchar_t scaleText[32] = {};
        swprintf_s(scaleText, L"%.3g", options.legacyDpiScaleFactor);
        args.push_back(std::wstring(L"--dpi-scale=") + scaleText);
    }
    if (options.agreeDelaySeconds > 0) {
        args.push_back(L"--agree-delay-seconds=" + std::to_wstring(options.agreeDelaySeconds));
    }
    if (options.prewarmOnly) {
        args.push_back(L"--prewarm");
    }
    return args;
}

std::vector<std::wstring> buildProtocolBrowserArguments(const std::wstring& title, const std::wstring& url)
{
    ProtocolBrowserLaunchOptions options;
    options.title = title;
    options.url = url;
    return buildProtocolBrowserArguments(options);
}

std::wstring protocolBrowserExecutablePath(const std::wstring& executableDirectory)
{
    return qtlogin::common::joinPath(executableDirectory, L"qtlogin_browser.exe");
}

bool launchProtocolBrowser(const ProtocolBrowserLaunchOptions& options)
{
    const std::wstring currentDirectory = qtlogin::common::currentExecutableDirectory();
    const std::wstring exePath = protocolBrowserExecutablePath(currentDirectory);
    std::wstring commandLine = qtlogin::common::quoteForCommandLine(exePath);
    for (const std::wstring& arg : buildProtocolBrowserArguments(options)) {
        commandLine += L" ";
        commandLine += qtlogin::common::quoteForCommandLine(arg);
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::wstring mutableCommandLine = commandLine;
    const ULONGLONG beginTick = GetTickCount64();
    qtlogin::common::logLine(
        L"sdologin",
        std::wstring(L"launch qtlogin_browser begin prewarm=") +
            (options.prewarmOnly ? L"1" : L"0") +
            L" exe=" + exePath +
            L" urlLen=" + std::to_wstring(options.url.size()));
    const BOOL created = CreateProcessW(
        exePath.c_str(),
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_PROCESS_GROUP,
        nullptr,
        currentDirectory.c_str(),
        &startup,
        &process);
    if (!created) {
        qtlogin::common::logLine(L"sdologin", L"failed to launch qtlogin_browser.exe error=" + std::to_wstring(GetLastError()));
        return false;
    }

    AllowSetForegroundWindow(process.dwProcessId);
    qtlogin::common::logLine(
        L"sdologin",
        L"launched qtlogin_browser.exe pid=" + std::to_wstring(process.dwProcessId) +
            L" elapsedMs=" + std::to_wstring(GetTickCount64() - beginTick));
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

bool launchProtocolBrowser(const std::wstring& title, const std::wstring& url)
{
    ProtocolBrowserLaunchOptions options;
    options.title = title;
    options.url = url;
    return launchProtocolBrowser(options);
}

}
