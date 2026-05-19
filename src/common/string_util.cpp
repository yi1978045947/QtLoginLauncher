#include "string_util.h"

#include <windows.h>

namespace qtlogin::common {

std::string wideToUtf8(const std::wstring& value)
{
    if (value.empty()) {
        return std::string();
    }

    const int sourceLength = static_cast<int>(value.size());
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), sourceLength, nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return std::string();
    }

    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), sourceLength, result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring utf8ToWide(const std::string& value)
{
    if (value.empty()) {
        return std::wstring();
    }

    const int sourceLength = static_cast<int>(value.size());
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), sourceLength, nullptr, 0);
    if (size <= 0) {
        return std::wstring();
    }

    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), sourceLength, result.data(), size);
    return result;
}

}
