#include <string>
#include <vector>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <windows.h>
#include <shellapi.h>

#include <QApplication>
#include <QDialog>
#include <QTimer>
#include <QString>
#include <QWidget>

#include "cef_offscreen_browser.h"
#include "protocol_browser_dialog.h"
#include "qt_dpi_config.h"

#ifdef QTLOGIN_ENABLE_CEF_OSR
#include "include/cef_app.h"
#endif

namespace {

bool startsWith(const std::wstring& value, const wchar_t* prefix)
{
    const std::wstring expected(prefix);
    return value.rfind(expected, 0) == 0;
}

std::wstring currentExecutableDirectory()
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
    return pos == std::wstring::npos ? L"." : path.substr(0, pos);
}

std::wstring timestamp()
{
    SYSTEMTIME st{};
    GetLocalTime(&st);
    std::wostringstream out;
    out << std::setfill(L'0')
        << st.wYear << L'-' << std::setw(2) << st.wMonth << L'-' << std::setw(2) << st.wDay
        << L' ' << std::setw(2) << st.wHour << L':' << std::setw(2) << st.wMinute
        << L':' << std::setw(2) << st.wSecond << L'.' << std::setw(3) << st.wMilliseconds;
    return out.str();
}

void browserLogLine(const std::wstring& message)
{
    std::wostringstream line;
    line << timestamp()
         << L" pid=" << GetCurrentProcessId()
         << L" tid=" << GetCurrentThreadId()
         << L" [qtlogin_browser] "
         << message << L"\r\n";
    const std::wstring text = line.str();
    OutputDebugStringW(text.c_str());

    const std::wstring path = currentExecutableDirectory() + L"\\qtlogin_rewrite.log";
    std::wofstream file(path, std::ios::app);
    if (file.is_open()) {
        file << text;
    }
}

std::vector<std::wstring> currentProcessArguments()
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::wstring> arguments;
    if (!argv) {
        return arguments;
    }

    arguments.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        arguments.emplace_back(argv[i]);
    }

    LocalFree(argv);
    return arguments;
}

struct BrowserOptions {
    std::wstring title = L"协议页面";
    std::wstring url = L"about:blank";
    HWND ownerHwnd = nullptr;
    double legacyDpiScaleFactor = 0.0;
    int agreeDelaySeconds = 0;
    bool prewarmOnly = false;
};

BrowserOptions parseOptions(const std::vector<std::wstring>& arguments)
{
    BrowserOptions options;
    const std::wstring titlePrefix = L"--title=";
    const std::wstring urlPrefix = L"--url=";
    const std::wstring ownerPrefix = L"--owner-hwnd=";
    const std::wstring dpiScalePrefix = L"--dpi-scale=";
    const std::wstring agreeDelayPrefix = L"--agree-delay-seconds=";
    for (size_t i = 1; i < arguments.size(); ++i) {
        const std::wstring& arg = arguments[i];
        if (startsWith(arg, titlePrefix.c_str())) {
            options.title = arg.substr(titlePrefix.size());
        } else if (startsWith(arg, urlPrefix.c_str())) {
            options.url = arg.substr(urlPrefix.size());
        } else if (startsWith(arg, ownerPrefix.c_str())) {
            try {
                options.ownerHwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(std::stoull(arg.substr(ownerPrefix.size()))));
            } catch (...) {
                options.ownerHwnd = nullptr;
            }
        } else if (startsWith(arg, dpiScalePrefix.c_str())) {
            options.legacyDpiScaleFactor = std::wcstod(arg.substr(dpiScalePrefix.size()).c_str(), nullptr);
        } else if (startsWith(arg, agreeDelayPrefix.c_str())) {
            try {
                options.agreeDelaySeconds = std::stoi(arg.substr(agreeDelayPrefix.size()));
            } catch (...) {
                options.agreeDelaySeconds = 0;
            }
        } else if (arg == L"--prewarm") {
            options.prewarmOnly = true;
        }
    }
    return options;
}

HWND stableOwnerWindow(HWND owner)
{
    if (!owner || !IsWindow(owner)) {
        return nullptr;
    }
    HWND root = GetAncestor(owner, GA_ROOT);
    return root && IsWindow(root) ? root : owner;
}

void activateProtocolDialog(QWidget* dialog, HWND owner)
{
    if (!dialog) {
        return;
    }
    HWND dialogHwnd = reinterpret_cast<HWND>(dialog->winId());
    HWND stableOwner = stableOwnerWindow(owner);
    if (stableOwner) {
        SetWindowLongPtrW(dialogHwnd, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(stableOwner));
        RECT ownerRect{};
        if (GetWindowRect(stableOwner, &ownerRect)) {
            const int ownerWidth = ownerRect.right - ownerRect.left;
            const int ownerHeight = ownerRect.bottom - ownerRect.top;
            const int x = ownerRect.left + std::max(0, (ownerWidth - dialog->width()) / 2);
            const int y = ownerRect.top + std::max(0, (ownerHeight - dialog->height()) / 2);
            SetWindowPos(dialogHwnd, HWND_TOP, x, y, dialog->width(), dialog->height(), SWP_SHOWWINDOW);
        }
    }
    ShowWindow(dialogHwnd, SW_SHOWNORMAL);
    SetForegroundWindow(dialogHwnd);
    dialog->raise();
    dialog->activateWindow();
}

}

int main(int argc, char* argv[])
{
#if defined(QTLOGIN_ENABLE_CEF_OSR) && defined(ARCH_CPU_32_BITS)
    const int cefStackExit = CefRunMainWithPreferredStackSize(main, argc, argv);
    if (cefStackExit >= 0) {
        return cefStackExit;
    }
#endif

    qtlogin::sdologin::configureQtLoginDpi();

#ifdef QTLOGIN_ENABLE_CEF_OSR
    const int cefExitCode = qtlogin::sdologin::executeCefSubProcess(GetModuleHandleW(nullptr));
    if (cefExitCode >= 0) {
        return cefExitCode;
    }
#endif

    const BrowserOptions options = parseOptions(currentProcessArguments());
#ifdef QTLOGIN_ENABLE_CEF_OSR
    browserLogLine(L"cef preinit begin");
    const bool cefInitialized = qtlogin::sdologin::ensureCefInitialized();
    browserLogLine(cefInitialized ? L"cef preinit succeeded" : L"cef preinit failed");
#endif
    QApplication app(argc, argv);
    browserLogLine(
        L"start title=" + options.title +
            L" urlLen=" + std::to_wstring(options.url.size()) +
            L" owner=" + std::to_wstring(reinterpret_cast<uintptr_t>(options.ownerHwnd)) +
            L" dpiScale=" + std::to_wstring(options.legacyDpiScaleFactor) +
            L" agreeDelay=" + std::to_wstring(options.agreeDelaySeconds) +
            L" prewarm=" + (options.prewarmOnly ? std::wstring(L"1") : std::wstring(L"0")));
    if (options.prewarmOnly) {
        const ULONGLONG beginTick = GetTickCount64();
        browserLogLine(
            L"prewarm loader-only elapsedMs=" +
            std::to_wstring(GetTickCount64() - beginTick));
        return 0;
    }
    browserLogLine(L"convert dialog strings begin");
    const QString dialogTitle = QString::fromWCharArray(options.title.c_str(), static_cast<int>(options.title.size()));
    const QString dialogUrl = QString::fromWCharArray(options.url.c_str(), static_cast<int>(options.url.size()));
    browserLogLine(L"convert dialog strings end");
    browserLogLine(L"creating protocol dialog");
    qtlogin::sdologin::ProtocolBrowserDialog dialog(
        dialogTitle,
        dialogUrl,
        options.legacyDpiScaleFactor,
        options.agreeDelaySeconds);
    browserLogLine(L"protocol dialog created");
    QTimer::singleShot(0, &dialog, [&dialog, owner = options.ownerHwnd]() {
        browserLogLine(L"activate protocol dialog");
        activateProtocolDialog(&dialog, owner);
    });
    browserLogLine(L"protocol dialog exec begin");
    const int dialogResult = dialog.exec();
    browserLogLine(L"exit result=" + std::to_wstring(dialogResult));
    return dialogResult == QDialog::Accepted ? 0 : 10;
}
