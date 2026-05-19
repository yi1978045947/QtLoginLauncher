#pragma once

#include <string>

namespace qtlogin::common {

void logLine(const std::wstring& component, const std::wstring& message);
void logLine(const wchar_t* component, const wchar_t* message);

}
