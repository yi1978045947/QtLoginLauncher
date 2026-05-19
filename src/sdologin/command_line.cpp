#include "command_line.h"

#include <cstdlib>

#include <windows.h>
#include <shellapi.h>

namespace qtlogin::sdologin {

namespace {

bool startsWith(const std::wstring& value, const wchar_t* prefix)
{
    const std::wstring expected(prefix);
    return value.rfind(expected, 0) == 0;
}

int parsePositiveInt(const std::wstring& value)
{
    wchar_t* end = nullptr;
    const long parsed = std::wcstol(value.c_str(), &end, 10);
    if (!end || *end != L'\0' || parsed < 0 || parsed > 600000) {
        return -1;
    }
    return static_cast<int>(parsed);
}

}

SdologinOptions parseCommandLineArguments(const std::vector<std::wstring>& arguments)
{
    SdologinOptions options;
    const std::wstring pipePrefix = L"--pipe=";
    const std::wstring autoPrefix = L"--auto-success-ms=";
    const std::wstring protocolPrefix = L"--standalone-protocol=";
    const std::wstring urlPrefix = L"--standalone-url=";

    for (size_t i = 1; i < arguments.size(); ++i) {
        const std::wstring& arg = arguments[i];
        if (startsWith(arg, pipePrefix.c_str())) {
            options.pipeName = arg.substr(pipePrefix.size());
        } else if (arg == L"--standalone-demo") {
            options.standaloneDemo = true;
        } else if (startsWith(arg, protocolPrefix.c_str())) {
            options.standaloneProtocol = arg.substr(protocolPrefix.size());
        } else if (startsWith(arg, urlPrefix.c_str())) {
            options.standaloneUrl = arg.substr(urlPrefix.size());
        } else if (arg == L"--mock-cancel") {
            options.mockCancel = true;
        } else if (arg == L"--mock-success") {
            options.mockSuccess = true;
        } else if (arg == L"--mock-exit-after-show") {
            options.mockExitAfterShow = true;
        } else if (startsWith(arg, autoPrefix.c_str())) {
            options.autoSuccessMs = parsePositiveInt(arg.substr(autoPrefix.size()));
        }
    }

    return options;
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

}
