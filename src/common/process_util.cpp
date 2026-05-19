#include "process_util.h"

#include <vector>

namespace qtlogin::common {

static std::wstring directoryOfPath(const std::wstring& path)
{
    const size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L".";
    }
    return path.substr(0, pos);
}

std::wstring moduleDirectory(HMODULE module)
{
    std::vector<wchar_t> buffer(MAX_PATH);
    DWORD size = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (size == buffer.size()) {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    if (size == 0) {
        return L".";
    }
    return directoryOfPath(std::wstring(buffer.data(), size));
}

std::wstring currentExecutableDirectory()
{
    return moduleDirectory(nullptr);
}

std::wstring joinPath(const std::wstring& left, const std::wstring& right)
{
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    const wchar_t last = left[left.size() - 1];
    if (last == L'\\' || last == L'/') {
        return left + right;
    }
    return left + L"\\" + right;
}

std::wstring quoteForCommandLine(const std::wstring& value)
{
    std::wstring result = L"\"";
    for (wchar_t ch : value) {
        if (ch == L'"') {
            result += L'\\';
        }
        result += ch;
    }
    result += L"\"";
    return result;
}

}
