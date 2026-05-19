#include "logger.h"

#include <windows.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

#include "process_util.h"

namespace qtlogin::common {
namespace {

std::mutex g_logMutex;

std::wstring logPath()
{
    return joinPath(currentExecutableDirectory(), L"qtlogin_rewrite.log");
}

std::wstring timestamp()
{
    SYSTEMTIME st{};
    GetLocalTime(&st);
    std::wostringstream out;
    out << std::setfill(L'0')
        << st.wYear << L'-' << std::setw(2) << st.wMonth << L'-' << std::setw(2) << st.wDay
        << L' ' << std::setw(2) << st.wHour << L':' << std::setw(2) << st.wMinute
        << L':' << std::setw(2) << st.wSecond << L'.' << std::setw(3) << st.wMilliseconds;
    return out.str();
}

}

void logLine(const std::wstring& component, const std::wstring& message)
{
    std::lock_guard<std::mutex> lock(g_logMutex);

    std::wostringstream line;
    line << timestamp()
         << L" pid=" << GetCurrentProcessId()
         << L" tid=" << GetCurrentThreadId()
         << L" [" << component << L"] "
         << message << L"\r\n";

    const std::wstring text = line.str();
    OutputDebugStringW(text.c_str());

    std::wofstream file(logPath(), std::ios::app);
    if (file.is_open()) {
        file << text;
    }
}

void logLine(const wchar_t* component, const wchar_t* message)
{
    logLine(std::wstring(component ? component : L""), std::wstring(message ? message : L""));
}

}
