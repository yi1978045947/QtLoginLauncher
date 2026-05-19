#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

using namespace std;

class WinHttpClient
{
public:
    WinHttpClient();
    ~WinHttpClient();

    int SendHttpRequest(const string& hostName, int port,
        const string& url, const string& method,
        const string& postData, int timeout,
        string& response, vector<string>& vecCookies, bool proxyEnable);

    int Cancel();

private:
    HINTERNET m_hSession;
    HINTERNET m_hConnect;
    HINTERNET m_hRequest;
    bool m_cancel;
};
