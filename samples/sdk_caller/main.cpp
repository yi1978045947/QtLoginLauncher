#include <iostream>

#include <windows.h>
#include <oleauto.h>

#include "legacy/interface.h"

namespace {

int CALLBACK loginCallback(int errorCode, const SDOLLoginResult* result, int userData, int)
{
    std::wcout << L"login callback error=" << errorCode << L" userData=" << userData << L"\n";
    if (result) {
        std::wcout << L"  sessionId=" << (result->SessionId ? result->SessionId : L"") << L"\n";
        std::wcout << L"  sndaid=" << (result->Sndaid ? result->Sndaid : L"") << L"\n";
        std::wcout << L"  identityState=" << (result->IdentityState ? result->IdentityState : L"") << L"\n";
    }
    return SDOL_LOGINRESULT_CLOSE;
}

}

int wmain()
{
    HMODULE sdk = LoadLibraryW(L"sdologinsdk.dll");
    if (!sdk) {
        std::wcerr << L"LoadLibrary(sdologinsdk.dll) failed: " << GetLastError() << L"\n";
        return 1;
    }

    auto initialize = reinterpret_cast<LPSDOLInitialize>(GetProcAddress(sdk, "SDOLInitialize"));
    auto getModule = reinterpret_cast<LPSDOLGetModule>(GetProcAddress(sdk, "SDOLGetModule"));
    auto terminal = reinterpret_cast<LPSDOLTerminal>(GetProcAddress(sdk, "SDOLTerminal"));
    if (!initialize || !getModule || !terminal) {
        std::wcerr << L"SDK exports are missing\n";
        FreeLibrary(sdk);
        return 2;
    }

    SDOLAppInfo appInfo{};
    appInfo.Size = sizeof(appInfo);
    appInfo.AppID = 1;
    appInfo.AppName = L"QtLoginRewrite Sample";
    appInfo.AppVer = L"1.0.0";
    appInfo.AreaId = -1;
    appInfo.GroupId = -1;

    int code = initialize(&appInfo);
    std::wcout << L"SDOLInitialize => " << code << L"\n";
    if (code != SDOL_ERRORCODE_OK) {
        FreeLibrary(sdk);
        return 3;
    }

    ISDOLLogin7* login = nullptr;
    code = getModule(__uuidof(ISDOLLogin7), reinterpret_cast<void**>(&login));
    std::wcout << L"SDOLGetModule(ISDOLLogin7) => " << code << L"\n";
    if (code != SDOL_ERRORCODE_OK || !login) {
        terminal();
        FreeLibrary(sdk);
        return 4;
    }

    login->SetOwnerWindow(GetConsoleWindow());
    login->SetLoginMode(NormalLoginMode);
    code = login->ShowLoginDialog(&loginCallback, 1234, 0);
    std::wcout << L"ShowLoginDialog => " << code << L"\n";

    BSTR ticket = nullptr;
    BSTR sndaid = nullptr;
    code = login->GetTicket(&ticket, &sndaid);
    std::wcout << L"GetTicket => " << code << L"\n";
    if (code == SDOL_ERRORCODE_OK) {
        std::wcout << L"  ticket=" << (ticket ? ticket : L"") << L"\n";
        std::wcout << L"  sndaid=" << (sndaid ? sndaid : L"") << L"\n";
    }
    if (ticket) {
        SysFreeString(ticket);
    }
    if (sndaid) {
        SysFreeString(sndaid);
    }

    login->Release();
    terminal();
    FreeLibrary(sdk);
    return code == SDOL_ERRORCODE_OK ? 0 : 5;
}
