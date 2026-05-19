#include "login_process_manager.h"

#include "logger.h"
#include "process_util.h"

namespace qtlogin::sdk {

namespace {

HMODULE currentModule()
{
    HMODULE module = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&currentModule),
        &module);
    return module;
}

}

LoginProcessManager::~LoginProcessManager()
{
    close();
}

std::wstring sdkModuleDirectory()
{
    return common::moduleDirectory(currentModule());
}

bool LoginProcessManager::start(const std::wstring& pipeName, const std::wstring& extraArguments)
{
    close();

    const std::wstring exePath = common::joinPath(sdkModuleDirectory(), L"sdologin.exe");
    std::wstring commandLine = common::quoteForCommandLine(exePath) + L" --pipe=" + pipeName + extraArguments;

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    if (!CreateProcessW(
            nullptr,
            commandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo_)) {
        common::logLine(L"sdk", L"failed to launch sdologin.exe");
        return false;
    }

    if (processInfo_.hThread) {
        CloseHandle(processInfo_.hThread);
        processInfo_.hThread = nullptr;
    }
    return true;
}

DWORD LoginProcessManager::wait(DWORD timeoutMs) const
{
    if (!processInfo_.hProcess) {
        return WAIT_FAILED;
    }
    return WaitForSingleObject(processInfo_.hProcess, timeoutMs);
}

HANDLE LoginProcessManager::processHandle() const
{
    return processInfo_.hProcess;
}

DWORD LoginProcessManager::processId() const
{
    return processInfo_.dwProcessId;
}

void LoginProcessManager::close()
{
    if (processInfo_.hThread) {
        CloseHandle(processInfo_.hThread);
        processInfo_.hThread = nullptr;
    }
    if (processInfo_.hProcess) {
        CloseHandle(processInfo_.hProcess);
        processInfo_.hProcess = nullptr;
    }
    processInfo_.dwProcessId = 0;
    processInfo_.dwThreadId = 0;
}

}
