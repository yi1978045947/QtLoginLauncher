#pragma once

#include <string>

#include <windows.h>

namespace qtlogin::common {

std::wstring moduleDirectory(HMODULE module);
std::wstring currentExecutableDirectory();
std::wstring joinPath(const std::wstring& left, const std::wstring& right);
std::wstring quoteForCommandLine(const std::wstring& value);

}
