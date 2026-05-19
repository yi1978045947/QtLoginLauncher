#pragma once

#include <string>

namespace qtlogin::sdologin {

bool makeLegacyPasswordTextBase64(
    const std::string& plainPasswordAnsi,
    const std::string& dynamicKey,
    std::string* output,
    std::wstring* errorMessage);

bool sdoaEncryptWithDynamicKey(
    const std::string& plain,
    const std::string& dynamicKey,
    std::string* output,
    std::wstring* errorMessage);

}
