#include "StdAfx.h"
#include "WinHttpClient.h"
#include "Tracer.h"

static std::string WinHttpClientLocalUnicodeToUtf8(const std::wstring& strUnicode)
{
#if defined(PLATFORM_WINDOWS)
    if (strUnicode.empty()) return "";

    int len = ::WideCharToMultiByte(
        CP_UTF8, 0,
        strUnicode.c_str(), -1,
        nullptr, 0, nullptr, nullptr);

    if (len <= 0) return "";

    std::string ret(len - 1, '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0,
        strUnicode.c_str(), -1,
        &ret[0], len,
        nullptr, nullptr);

    return ret;
#else
    // 非 Windows 才用 wchar_t == UTF-32 的手写逻辑
    std::string ret;
    for (wchar_t c : strUnicode)
    {
        uint32_t uc = static_cast<uint32_t>(c);

        if (uc < 0x80)
        {
            ret.push_back(static_cast<char>(uc));
        }
        else if (uc < 0x0800)
        {
            ret.push_back(static_cast<char>((uc >> 6) | 0xC0));
            ret.push_back(static_cast<char>((uc & 0x3F) | 0x80));
        }
        else if (uc < 0x010000)
        {
            ret.push_back(static_cast<char>((uc >> 12) | 0xE0));
            ret.push_back(static_cast<char>(((uc >> 6) & 0x3F) | 0x80));
            ret.push_back(static_cast<char>((uc & 0x3F) | 0x80));
        }
        else
        {
            ret.push_back(static_cast<char>((uc >> 18) | 0xF0));
            ret.push_back(static_cast<char>(((uc >> 12) & 0x3F) | 0x80));
            ret.push_back(static_cast<char>(((uc >> 6) & 0x3F) | 0x80));
            ret.push_back(static_cast<char>((uc & 0x3F) | 0x80));
        }
    }
    return ret;
#endif
}

WinHttpClient::WinHttpClient()
    : m_hSession(nullptr)
    , m_hConnect(nullptr)
    , m_hRequest(nullptr)
    , m_cancel(false)
{
}

WinHttpClient::~WinHttpClient()
{
    Cancel();
}

int WinHttpClient::SendHttpRequest(const string& hostName, int port,
    const string& urlPath, const string& method,
    const string& postData, int timeout,
    string& response, vector<string>& vecCookies,
    bool proxyEnable)
{
    TRACET();

    m_cancel = false;
    response.clear();
    vecCookies.clear();

    // 打开 WinHTTP 会话（UA 修正：去掉多余空格）
    m_hSession = WinHttpOpen(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64; Trident/7.0; rv:11.0) like Gecko",
        proxyEnable ? WINHTTP_ACCESS_TYPE_DEFAULT_PROXY : WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (!m_hSession)
    {
        TRACEE(L"WinHttpOpen failed. err=%d\n", GetLastError());
        return -7;
    }

    DWORD timeoutMs = timeout;
    WinHttpSetTimeouts(m_hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    // 转换主机名
    // wstring hostW(hostName.begin(), hostName.end());
    wstring hostW;
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, hostName.c_str(), -1, nullptr, 0);
        hostW.resize(len > 0 ? len - 1 : 0);
        if (len > 0)
            MultiByteToWideChar(CP_UTF8, 0, hostName.c_str(), -1, &hostW[0], len);
    }
    m_hConnect = WinHttpConnect(m_hSession, hostW.c_str(), port, 0);
    if (!m_hConnect)
    {
        TRACEE(L"WinHttpConnect failed. err=%d\n", GetLastError());
        Cancel();
        return -2;
    }

    // 构造 URL 路径
    //wstring urlW(urlPath.begin(), urlPath.end());
    wstring urlW;
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, urlPath.c_str(), -1, nullptr, 0);
        urlW.resize(len > 0 ? len - 1 : 0);
        if (len > 0)
            MultiByteToWideChar(CP_UTF8, 0, urlPath.c_str(), -1, &urlW[0], len);
    }

    //wstring methodW(method.begin(), method.end());
    wstring methodW;
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, method.c_str(), -1, nullptr, 0);
        methodW.resize(len > 0 ? len - 1 : 0);
        if (len > 0)
            MultiByteToWideChar(CP_UTF8, 0, method.c_str(), -1, &methodW[0], len);
    }

    DWORD dwFlags = WINHTTP_FLAG_REFRESH;
    if (port == INTERNET_DEFAULT_HTTPS_PORT)
        dwFlags |= WINHTTP_FLAG_SECURE;

    m_hRequest = WinHttpOpenRequest(m_hConnect, methodW.c_str(), urlW.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, dwFlags);

    if (!m_hRequest)
    {
        TRACEE(L"WinHttpOpenRequest failed. err=%d\n", GetLastError());
        Cancel();
        return -3;
    }

    // 设置 SSL 忽略（如果是 HTTPS）
    if (dwFlags & WINHTTP_FLAG_SECURE)
    {
        DWORD dwSecFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
            SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
            SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
            SECURITY_FLAG_IGNORE_REVOCATION;
        WinHttpSetOption(m_hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwSecFlags, sizeof(dwSecFlags));
    }

    // 发送请求
    BOOL bSend = WinHttpSendRequest(m_hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        postData.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)postData.c_str(),
        (DWORD)postData.length(),
        (DWORD)postData.length(),
        0);

    if (!bSend)
    {
        TRACEE(L"WinHttpSendRequest failed. err=%d\n", GetLastError());
        Cancel();
        return -4;
    }

    if (!WinHttpReceiveResponse(m_hRequest, nullptr))
    {
        TRACEE(L"WinHttpReceiveResponse failed. err=%d\n", GetLastError());
        Cancel();
        return -5;
    }

    // 获取状态码
    DWORD dwStatus = 0;
    DWORD dwSize = sizeof(dwStatus);
    if (!WinHttpQueryHeaders(m_hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &dwStatus, &dwSize, WINHTTP_NO_HEADER_INDEX))
    {
        TRACEE(L"WinHttpQueryHeaders failed. err=%d\n", GetLastError());
        Cancel();
        return -6;
    }

    if (dwStatus != 200)
    {
        TRACEE(L"Http status code=%d\n", dwStatus);
        Cancel();
        return dwStatus;
    }

    // 兼容旧逻辑：读取 Set-Cookie
    DWORD dwCookieSize = 0;
    WinHttpQueryHeaders(
        m_hRequest,
        WINHTTP_QUERY_SET_COOKIE,
        WINHTTP_HEADER_NAME_BY_INDEX,
        WINHTTP_NO_OUTPUT_BUFFER,
        &dwCookieSize,
        WINHTTP_NO_HEADER_INDEX);

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && dwCookieSize > 0)
    {
        std::vector<wchar_t> cookieBuf(dwCookieSize / sizeof(wchar_t));
        if (WinHttpQueryHeaders(
            m_hRequest,
            WINHTTP_QUERY_SET_COOKIE,
            WINHTTP_HEADER_NAME_BY_INDEX,
            cookieBuf.data(),
            &dwCookieSize,
            WINHTTP_NO_HEADER_INDEX))
        {
            std::wstring ws(cookieBuf.data());
            std::string cookie = WinHttpClientLocalUnicodeToUtf8(ws);
            vecCookies.push_back(cookie);
        }
    }

    // 读取响应体
    DWORD dwDownloaded = 0;
    do
    {
        DWORD dwAvail = 0;
        if (!WinHttpQueryDataAvailable(m_hRequest, &dwAvail))
        {
            TRACEE(L"WinHttpQueryDataAvailable failed. err=%d\n", GetLastError());
            break;
        }

        if (dwAvail == 0)
            break;

        std::vector<char> buffer(dwAvail + 1);
        ZeroMemory(buffer.data(), buffer.size());

        if (!WinHttpReadData(m_hRequest, buffer.data(), dwAvail, &dwDownloaded))
        {
            TRACEE(L"WinHttpReadData failed. err=%d\n", GetLastError());
            break;
        }

        if (dwDownloaded > 0)
        {
            response.append(buffer.data(), dwDownloaded);
        }

    } while (dwDownloaded > 0 && !m_cancel);

    Cancel();
    return 0;
}

int WinHttpClient::Cancel()
{
    m_cancel = true;

    if (m_hRequest)
    {
        WinHttpCloseHandle(m_hRequest);
        m_hRequest = nullptr;
    }
    if (m_hConnect)
    {
        WinHttpCloseHandle(m_hConnect);
        m_hConnect = nullptr;
    }
    if (m_hSession)
    {
        WinHttpCloseHandle(m_hSession);
        m_hSession = nullptr;
    }
    return 0;
}
