#pragma once

#include <string>

namespace qtlogin::common {

std::string wideToUtf8(const std::wstring& value);
std::wstring utf8ToWide(const std::string& value);

}
