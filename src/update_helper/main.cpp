#include <string>

#include <windows.h>
#include <shellapi.h>

#include "logger.h"

namespace {

std::wstring mutexNameFromCommandLine()
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        return std::wstring();
    }

    std::wstring mutexName;
    for (int i = 1; i < argc; ++i) {
        const std::wstring arg(argv[i]);
        const std::wstring prefix = L"--mutex=";
        if (arg.rfind(prefix, 0) == 0) {
            mutexName = arg.substr(prefix.size());
            break;
        }
    }

    LocalFree(argv);
    return mutexName;
}

}

int wmain()
{
    const std::wstring mutexName = mutexNameFromCommandLine();
    HANDLE mutexHandle = nullptr;
    if (!mutexName.empty()) {
        mutexHandle = CreateMutexW(nullptr, FALSE, mutexName.c_str());
    }

    qtlogin::common::logLine(L"update", L"update helper started");

    if (mutexHandle) {
        CloseHandle(mutexHandle);
    }
    return 0;
}
