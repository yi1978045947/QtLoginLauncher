#pragma once

#include <string>

#include <windows.h>

namespace qtlogin::sdk {

class LoginProcessManager {
public:
    LoginProcessManager() = default;
    ~LoginProcessManager();

    LoginProcessManager(const LoginProcessManager&) = delete;
    LoginProcessManager& operator=(const LoginProcessManager&) = delete;

    bool start(const std::wstring& pipeName, const std::wstring& extraArguments = std::wstring());
    DWORD wait(DWORD timeoutMs) const;
    HANDLE processHandle() const;
    DWORD processId() const;
    void close();

private:
    PROCESS_INFORMATION processInfo_{};
};

std::wstring sdkModuleDirectory();

}
