#include "stdafx.h"
#include "PathHelper.h"
#include "LogManager.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

HMODULE g_hModule = NULL;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        PathHelper::Initial(hModule);
        CLogManager::InitialLog(hModule);

        {
            WSADATA wsaData;
            int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (ret != 0)
            {
                OutputDebugStringA("WSAStartup failed in DLL_PROCESS_ATTACH");
            }
        }
        break;

    case DLL_PROCESS_DETACH:
        // 只在进程卸载 DLL 时调用一次
        WSACleanup();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        // 不能在这里 WSACleanup！！
        break;
    }

    return TRUE;
}
