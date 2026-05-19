#pragma once

#include <string>
#include <vector>

namespace qtlogin::sdologin {

struct SdologinOptions {
    std::wstring pipeName;
    bool standaloneDemo = false;
    std::wstring standaloneProtocol;
    std::wstring standaloneUrl;
    bool mockCancel = false;
    bool mockSuccess = false;
    bool mockExitAfterShow = false;
    int autoSuccessMs = -1;
};

SdologinOptions parseCommandLineArguments(const std::vector<std::wstring>& arguments);
std::vector<std::wstring> currentProcessArguments();

}
