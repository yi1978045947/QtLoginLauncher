#include <windows.h>

#include "logger.h"

extern "C" int WINAPI GameUpdateInitialize()
{
    qtlogin::common::logLine(L"GameUpdate", L"GameUpdateInitialize");
    return 0;
}

extern "C" int WINAPI GameUpdateTerminal()
{
    qtlogin::common::logLine(L"GameUpdate", L"GameUpdateTerminal");
    return 0;
}
