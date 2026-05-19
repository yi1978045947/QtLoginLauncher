/**
 * CurlHttpClient.cpp
 * --------------------------------------------------------------------
 * 实现基于 libcurl + OpenSSL 的 HTTP 客户端，用于完全替代原 WinInet 实现。
 *
 * 特点：
 *   不依赖 WinInet，不受系统 TLS 勾选影响
 *   支持 TLS1.0/1.1/1.2/1.3，底层由 OpenSSL 提供安全能力
 *   行为最大限度保持与旧 HttpClient 一致（返回值、流程、超时）
 *   支持 Cancel 取消请求（通过 XFERINFO 回调中断）
 *   支持 GET / POST / 其他自定义方法
 *   支持服务器返回的 Set-Cookie
 *   支持代理开关（与旧 HttpClient 行为一致）
 *   完全跨平台（但此项目用途为 Windows）
 *
 * 注意：
 *   - 本类是线程安全的，只要每个线程有自己的 CurlHttpClient 实例
 *   - 不建议多个线程共享同一个 m_curl
 */

 // ============================================================================
 // HTTP Network Layer
 //
 // Previous implementation:
 //   WinInet
 //
 // Current implementation:
 //   libcurl + OpenSSL
 //
 // Build Toolchain:
 //   Visual Studio 2019
 //
 // Library Type:
 //   Dynamic Libraries (DLL)
 //
 // Advantages:
 //   - Avoid Windows Schannel TLS limitations
 //   - Better HTTPS compatibility
 //   - More detailed network error detection
 //     * DNS error
 //     * TCP connection error
 //     * TLS handshake error
 //     * HTTP timeout
 //
 // Error Mapping Example:
 //   -2 : DNS resolve failed
 //   -3 : TCP connect failed
 //   -4 : TLS handshake failed
 // ============================================================================

#include "StdAfx.h"
#include "CurlHttpClient.h"
#include <sstream>
#include <iostream>
#include <cstdarg>
#include "Tracer.h"

 // ======================================================
  // 全局 CURL 错误描述映射表
  // ======================================================
const std::map<int, CurlHttpClient::CurlErrInfo> CurlHttpClient::g_curlErrMap =
{
	{ 0,  {"CURLE_OK", "No error"} },
	{ 1,  {"CURLE_UNSUPPORTED_PROTOCOL", "Unsupported protocol"} },
	{ 2,  {"CURLE_FAILED_INIT", "Failed to initialize"} },
	{ 3,  {"CURLE_URL_MALFORMAT", "URL malformatted"} },
	{ 4,  {"CURLE_NOT_BUILT_IN", "Feature not built-in"} },
	{ 5,  {"CURLE_COULDNT_RESOLVE_PROXY", "Could not resolve proxy host"} },
	{ 6,  {"CURLE_COULDNT_RESOLVE_HOST", "Could not resolve host"} },
	{ 7,  {"CURLE_COULDNT_CONNECT", "Failed to connect to server"} },
	{ 8,  {"CURLE_WEIRD_SERVER_REPLY", "Strange server reply"} },
	{ 9,  {"CURLE_REMOTE_ACCESS_DENIED", "Access denied on remote server"} },

	{ 10, {"CURLE_FTP_ACCEPT_FAILED", "FTP: failed to accept data connection"} },
	{ 11, {"CURLE_FTP_WEIRD_PASS_REPLY", "FTP: unexpected PASS reply"} },
	{ 12, {"CURLE_FTP_ACCEPT_TIMEOUT", "FTP: accept timeout"} },
	{ 13, {"CURLE_FTP_WEARD_PASV_REPLY", "FTP: invalid PASV reply"} },
	{ 14, {"CURLE_FTP_WEIRD_227_FORMAT", "FTP: invalid 227 format"} },
	{ 15, {"CURLE_FTP_CANT_GET_HOST", "FTP: cannot retrieve host"} },
	{ 16, {"CURLE_HTTP2", "HTTP/2 framing layer error"} },

	{ 17, {"CURLE_FTP_COULDNT_SET_TYPE", "FTP: failed to set file transfer type"} },
	{ 18, {"CURLE_PARTIAL_FILE", "Partial file (incomplete download)"} },
	{ 19, {"CURLE_FTP_COULDNT_RETR_FILE", "FTP: failed to retrieve file"} },
	{ 20, {"CURLE_OBSOLETE20", "Obsolete"} },
	{ 21, {"CURLE_QUOTE_ERROR", "Quote command failed"} },
	{ 22, {"CURLE_HTTP_RETURNED_ERROR", "HTTP returned an error"} },
	{ 23, {"CURLE_WRITE_ERROR", "Write callback failed"} },
	{ 24, {"CURLE_OBSOLETE24", "Obsolete"} },

	{ 25, {"CURLE_UPLOAD_FAILED", "Upload failed"} },
	{ 26, {"CURLE_READ_ERROR", "Read callback error"} },
	{ 27, {"CURLE_OUT_OF_MEMORY", "Out of memory"} },
	{ 28, {"CURLE_OPERATION_TIMEDOUT", "Operation timed out"} },
	{ 29, {"CURLE_OBSOLETE29", "Obsolete"} },

	{ 30, {"CURLE_FTP_PORT_FAILED", "FTP: PORT command failed"} },
	{ 31, {"CURLE_FTP_COULDNT_USE_REST", "FTP: REST command failed"} },
	{ 32, {"CURLE_OBSOLETE32", "Obsolete"} },

	{ 33, {"CURLE_RANGE_ERROR", "Range request did not work"} },
	{ 34, {"CURLE_OBSOLETE34", "Obsolete"} },
	{ 35, {"CURLE_SSL_CONNECT_ERROR", "SSL/TLS handshake failed"} },
	{ 36, {"CURLE_BAD_DOWNLOAD_RESUME", "Could not resume download"} },
	{ 37, {"CURLE_FILE_COULDNT_READ_FILE", "Could not read file"} },

	{ 38, {"CURLE_LDAP_CANNOT_BIND", "LDAP: cannot bind"} },
	{ 39, {"CURLE_LDAP_SEARCH_FAILED", "LDAP: search failed"} },

	{ 40, {"CURLE_OBSOLETE40", "Obsolete"} },
	{ 41, {"CURLE_OBSOLETE41", "Obsolete"} },

	{ 42, {"CURLE_ABORTED_BY_CALLBACK", "Operation aborted by callback"} },
	{ 43, {"CURLE_BAD_FUNCTION_ARGUMENT", "Invalid function argument"} },
	{ 44, {"CURLE_OBSOLETE44", "Obsolete"} },
	{ 45, {"CURLE_INTERFACE_FAILED", "Interface error"} },
	{ 46, {"CURLE_OBSOLETE46", "Obsolete"} },
	{ 47, {"CURLE_TOO_MANY_REDIRECTS", "Too many redirects"} },
	{ 48, {"CURLE_UNKNOWN_OPTION", "Unknown option"} },
	{ 49, {"CURLE_SETOPT_OPTION_SYNTAX", "Malformed option"} },

	{ 50, {"CURLE_OBSOLETE50", "Obsolete"} },
	{ 51, {"CURLE_OBSOLETE51", "Obsolete"} },
	{ 52, {"CURLE_GOT_NOTHING", "Server returned no data"} },

	{ 53, {"CURLE_SSL_ENGINE_NOTFOUND", "SSL engine not found"} },
	{ 54, {"CURLE_SSL_ENGINE_SETFAILED", "Failed to set SSL engine"} },

	{ 55, {"CURLE_SEND_ERROR", "Failed sending network data"} },
	{ 56, {"CURLE_RECV_ERROR", "Failed receiving network data"} },

	{ 57, {"CURLE_OBSOLETE57", "Obsolete"} },
	{ 58, {"CURLE_SSL_CERTPROBLEM", "SSL: local certificate problem"} },
	{ 59, {"CURLE_SSL_CIPHER", "SSL: cipher error"} },

	{ 60, {"CURLE_PEER_FAILED_VERIFICATION", "SSL: peer verification failed"} },
	{ 61, {"CURLE_BAD_CONTENT_ENCODING", "Bad content encoding"} },
	{ 62, {"CURLE_OBSOLETE62", "Obsolete"} },
	{ 63, {"CURLE_FILESIZE_EXCEEDED", "Maximum file size exceeded"} },
	{ 64, {"CURLE_USE_SSL_FAILED", "Requested SSL level failed"} },
	{ 65, {"CURLE_SEND_FAIL_REWIND", "Rewind send failed"} },

	{ 66, {"CURLE_SSL_ENGINE_INITFAILED", "Failed to initialize SSL engine"} },
	{ 67, {"CURLE_LOGIN_DENIED", "Login denied"} },

	{ 68, {"CURLE_TFTP_NOTFOUND", "TFTP: file not found"} },
	{ 69, {"CURLE_TFTP_PERM", "TFTP: permission problem"} },
	{ 70, {"CURLE_REMOTE_DISK_FULL", "Remote disk full"} },
	{ 71, {"CURLE_TFTP_ILLEGAL", "TFTP: illegal operation"} },
	{ 72, {"CURLE_TFTP_UNKNOWNID", "TFTP: unknown ID"} },
	{ 73, {"CURLE_REMOTE_FILE_EXISTS", "Remote file exists"} },
	{ 74, {"CURLE_TFTP_NOSUCHUSER", "TFTP: no such user"} },

	{ 75, {"CURLE_OBSOLETE75", "Obsolete"} },
	{ 76, {"CURLE_OBSOLETE76", "Obsolete"} },
	{ 77, {"CURLE_SSL_CACERT_BADFILE", "SSL: bad CA cert file"} },

	{ 78, {"CURLE_REMOTE_FILE_NOT_FOUND", "Remote file not found"} },
	{ 79, {"CURLE_SSH", "SSH error"} },
	{ 80, {"CURLE_SSL_SHUTDOWN_FAILED", "SSL shutdown failed"} },

	{ 81, {"CURLE_AGAIN", "Socket not ready (try again later)"} },
	{ 82, {"CURLE_SSL_CRL_BADFILE", "SSL: bad CRL file"} },
	{ 83, {"CURLE_SSL_ISSUER_ERROR", "SSL: issuer check failed"} },

	{ 84, {"CURLE_FTP_PRET_FAILED", "FTP: PRET command failed"} },
	{ 85, {"CURLE_RTSP_CSEQ_ERROR", "RTSP CSeq mismatch"} },
	{ 86, {"CURLE_RTSP_SESSION_ERROR", "RTSP session mismatch"} },
	{ 87, {"CURLE_FTP_BAD_FILE_LIST", "FTP: bad file list"} },
	{ 88, {"CURLE_CHUNK_FAILED", "Chunk callback failed"} },
	{ 89, {"CURLE_NO_CONNECTION_AVAILABLE", "No connection available"} },

	{ 90, {"CURLE_SSL_PINNEDPUBKEYNOTMATCH", "Pinned public key mismatch"} },
	{ 91, {"CURLE_SSL_INVALIDCERTSTATUS", "Invalid certificate status"} },
	{ 92, {"CURLE_HTTP2_STREAM", "HTTP/2 stream error"} },

	{ 93, {"CURLE_RECURSIVE_API_CALL", "Recursive API call error"} },
	{ 94, {"CURLE_AUTH_ERROR", "Authentication error"} },
	{ 95, {"CURLE_HTTP3", "HTTP/3 error"} },
	{ 96, {"CURLE_QUIC_CONNECT_ERROR", "QUIC connection error"} },
	{ 97, {"CURLE_PROXY", "Proxy handshake error"} },
	{ 98, {"CURLE_SSL_CLIENTCERT", "Client certificate required"} },
	{ 99, {"CURLE_UNRECOVERABLE_POLL", "Poll/select fatal error"} },
	{100, {"CURLE_TOO_LARGE", "Data too large"} },
	{101, {"CURLE_ECH_REQUIRED", "ECH required but failed"} },
};

///////////////////////////////////////////////////////////////////////////////
// WinInet 风格 step 映射
///////////////////////////////////////////////////////////////////////////////

std::string CurlHttpClient::StepToString(CurlStep s)
{
	switch (s)
	{
	case STEP_DNS:  return "STEP_DNS";
	case STEP_TCP:  return "STEP_TCP";
	case STEP_TLS:  return "STEP_TLS";
	case STEP_SEND: return "STEP_SEND";
	case STEP_RECV: return "STEP_RECV";
	case STEP_DONE: return "STEP_DONE";
	default:        return "STEP_NONE";
	}
}

///////////////////////////////////////////////////////////////////////////////
// 统一的 step 日志输出（英文）
// WinInet 风格：TRACEE(L"STEP_SEND failed,dwInternetStatus=%d", v)
///////////////////////////////////////////////////////////////////////////////

void CurlHttpClient::LogStep(const char* fmt, ...)
{
	char buf[1024];
	va_list args;

	va_start(args, fmt);
	_vsnprintf_s(buf, sizeof(buf), fmt, args);
	va_end(args);

	TRACEI("[CURL] %s", buf); // Info 日志，用于正常 step 流程
}

///////////////////////////////////////////////////////////////////////////////
// curl debug callback —— 模拟 WinInet InternetStatusCallback
///////////////////////////////////////////////////////////////////////////////

int CurlHttpClient::DebugCallback(
	CURL* handle,
	curl_infotype type,
	char* data,
	size_t size,
	void* userptr)
{
	CurlHttpClient* self = reinterpret_cast<CurlHttpClient*>(userptr);
	std::string text(data, size);

	// DNS
	if (text.find("Trying ") != std::string::npos)
	{
		self->m_step = STEP_DNS;

		// 打印 curl 正在解析的真实域名（非常关键）
		TRACEI("[CURL] REAL DNS HOST = %s", text.c_str());
		TRACEI("[CURL] STEP_DNS ...");
	}

	// TCP Connect
	if (text.find("Connected to") != std::string::npos)
	{
		self->m_step = STEP_TCP;
		TRACEI("[CURL] STEP_TCP ...");
	}

	// TLS handshake
	if (text.find("SSL connection using") != std::string::npos ||
		text.find("SSL certificate verify ok") != std::string::npos)
	{
		self->m_step = STEP_TLS;
		TRACEI("[CURL] STEP_TLS ...");
	}

	// SEND
	if (type == CURLINFO_DATA_OUT || type == CURLINFO_HEADER_OUT)
	{
		self->m_step = STEP_SEND;
		TRACEI("[CURL] STEP_SEND ...");
	}

	// RECV
	if (type == CURLINFO_DATA_IN || type == CURLINFO_HEADER_IN)
	{
		self->m_step = STEP_RECV;
		TRACEI("[CURL] STEP_RECV ...");
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// 构造 / 析构
///////////////////////////////////////////////////////////////////////////////

CurlHttpClient::CurlHttpClient()
	: m_curl(nullptr)
	, m_cancelled(false)
	, m_step(STEP_NONE)
	, m_verifyCert(true)
	//, m_resolveList(nullptr)
{
	// ① 创建 handle
	m_curl = curl_easy_init();

	// ------------------------------
	// Curl 版本输出（关键诊断）
	// ------------------------------
	curl_version_info_data* v = curl_version_info(CURLVERSION_NOW);

	// 注意：
	// 这里统一使用 ANSI TRACE（窄字符串），
	// 因为 v->xxx 全部是 char*，避免宽窄混用导致潜在风险

	TRACEI("[CURL] version           = %s", v->version);
	TRACEI("[CURL] ssl_version       = %s", v->ssl_version);
	TRACEI("[CURL] libz_version      = %s",
		v->libz_version ? v->libz_version : "(none)");

	// 特性检测：ASYNCHDNS 是否开启？
	TRACEI("[CURL] ASYNCHDNS       = %s",
		(v->features & CURL_VERSION_ASYNCHDNS) ? "ON" : "OFF");

	// SSL 是否开启
	TRACEI("[CURL] SSL support     = %s",
		(v->features & CURL_VERSION_SSL) ? "YES" : "NO");

	// IDN 支持
	TRACEI("[CURL] IDN support     = %s",
		(v->features & CURL_VERSION_IDN) ? "YES" : "NO");
}

CurlHttpClient::~CurlHttpClient()
{
	// ① 先释放 DNS 绑定表（防止泄露 / DNS 混乱）
	//if (m_resolveList)
	//{
	//	curl_slist_free_all(m_resolveList);
	//	m_resolveList = nullptr;
	//}

	// ② 再释放 CURL handle
	if (m_curl)
	{
		curl_easy_cleanup(m_curl);
		m_curl = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Cancel
///////////////////////////////////////////////////////////////////////////////

int CurlHttpClient::Cancel()
{
	TRACET();
	m_cancelled = true;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// 设置 TLS 证书校验开关
///////////////////////////////////////////////////////////////////////////////
void CurlHttpClient::SetVerifyCert(bool enable)
{
	/**
	 * m_verifyCert 用于控制 SendHttpRequest() 是否启用证书验证。
	 *
	 * 说明：
	 *   - 该值不会立即配置到 curl_easy_setopt()
	 *   - 而是在每次 SendHttpRequest() 时，根据 m_verifyCert 进行设置
	 *   - 这样能确保不同请求可用不同的校验策略（与 WinInet 时代的行为一致）
	 */
	m_verifyCert = enable;
}

///////////////////////////////////////////////////////////////////////////////
// libcurl 回调
///////////////////////////////////////////////////////////////////////////////

size_t CurlHttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t total = size * nmemb;
	std::string* resp = reinterpret_cast<std::string*>(userp);
	resp->append(reinterpret_cast<char*>(contents), total);
	return total;
}

size_t CurlHttpClient::HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata)
{
	size_t total = size * nitems;
	vector<string>* cookies = reinterpret_cast<vector<string>*>(userdata);

	std::string header(buffer, total);
	const std::string prefix = "Set-Cookie:";

	if (header.size() > prefix.size() &&
		_strnicmp(header.c_str(), prefix.c_str(), prefix.size()) == 0)
	{
		std::string cookie = header.substr(prefix.size());

		while (!cookie.empty() && (cookie[0] == ' ' || cookie[0] == '\t'))
			cookie.erase(cookie.begin());

		while (!cookie.empty() && (cookie.back() == '\r' || cookie.back() == '\n'))
			cookie.pop_back();

		cookies->push_back(cookie);
	}

	return total;
}

int CurlHttpClient::ProgressCallback(void* clientp,
	curl_off_t dltotal, curl_off_t dlnow,
	curl_off_t ultotal, curl_off_t ulnow)
{
	CurlHttpClient* client = static_cast<CurlHttpClient*>(clientp);
	if (client->m_cancelled)
		return 1;
	return 0;
}

/******************************************************************************
 * Curl error mapping table (WinInet compatible)
 * ---------------------------------------------------------------------------
 * This project only uses HTTPS + GET/POST. Therefore, only a very small
 * subset of CURLcode values are relevant. All other protocol-related errors
 * (FTP/SSH/LDAP/FILE/etc.) will never occur and are ignored.
 *
 * This table shows the ONLY curl errors that LoginSDK actually needs.
 *
 *   CURLcode                          WinInet Step        Return Code
 * ---------------------------------------------------------------------------
 *   CURLE_COULDNT_RESOLVE_HOST   (6)  STEP_DNS           -1   // DNS failed
 *   CURLE_COULDNT_CONNECT        (7)  STEP_TCP           -1   // TCP connect failed
 *   CURLE_SSL_CONNECT_ERROR      (35) STEP_TLS           -1   // TLS handshake failed
 *   CURLE_PEER_FAILED_VERIFICATION(60)STEP_TLS           -1   // Certificate verify failed
 *
 *   CURLE_SEND_ERROR             (55) STEP_SEND          -3   // Failed sending data
 *   CURLE_UPLOAD_FAILED          (25) STEP_SEND          -3   // Failed sending/uploading
 *
 *   CURLE_RECV_ERROR             (56) STEP_RECV          -4   // Failed receiving data
 *
 *   CURLE_OPERATION_TIMEDOUT     (28) Timeout            -5   // Operation timed out
 *
 *   Cancel() triggered                STEP_SEND          -6   // User cancelled
 *
 *   HTTP code != 200                  STEP_RECV          -7   // Server returned error
 *
 *   All other errors                  (N/A)              -1   // Treat as connection failure
 *
 * Notes:
 *   - Mapping strictly follows old WinInet HttpClient behavior.
 *   - Keys: DNS → TCP → TLS → SEND → RECV → DONE.
 *   - Unknown curl errors are treated as STEP_CONN failure (-1).
 ******************************************************************************/

 /*******************************************************************************
  * Curl → WinInet 错误码映射表（保持与旧 HttpClient 完全一致）
  *
  * 本 SDK 仅使用 HTTPS + GET/POST，因此只需要处理以下有限的 curl 错误。
  * 映射结果必须与旧 WinInet HttpClient (-1 ~ -7) 完全一致。
  *
  *   CURLcode                          WinInet Step        Return
  * ---------------------------------------------------------------------------
  *   CURLE_COULDNT_RESOLVE_HOST   (6)  STEP_DNS           -1
  *       DNS 解析失败
  *
  *   CURLE_COULDNT_CONNECT        (7)  STEP_TCP           -1
  *       TCP 连接失败
  *
  *   CURLE_SSL_CONNECT_ERROR      (35) STEP_TLS           -1
  *   CURLE_PEER_FAILED_VERIFICATION(60)STEP_TLS           -1
  *       TLS 握手失败（证书错误 / TLS 不兼容）
  *
  *   CURLE_SEND_ERROR             (55) STEP_SEND          -3
  *   CURLE_UPLOAD_FAILED          (25) STEP_SEND          -3
  *       发送数据失败（对应 HttpSendRequest）
  *
  *   CURLE_RECV_ERROR             (56) STEP_RECV          -4
  *       接收数据失败
  *
  *   CURLE_OPERATION_TIMEDOUT     (28) timeout            -5
  *       连接或传输超时（WinInet 所有超时都映射为 -5）
  *
  *   Cancel() 触发                    STEP_SEND          -6
  *       用户主动取消请求
  *
  *   HTTP code != 200                  STEP_RECV          -6
  *       WinInet 中 HttpQueryInfo 状态码不为 200 → -6
  *
  *   其他所有错误                     (N/A)              -1
  *       统一视为连接阶段失败
  ******************************************************************************/


///////////////////////////////////////////////////////////////////////////////
// SendHttpRequest —— 核心实现
///////////////////////////////////////////////////////////////////////////////
int CurlHttpClient::SendHttpRequest(
	const string& hostName,
	int port,
	const string& url,
	const string& method,
	const string& postData,
	int timeout,
	string& response,
	vector<string>& vecCookies,
	bool proxyEnable)
{
	// =========================================================
	// ① 清空旧的 resolve list（一定要做）
	// =========================================================
	//if (m_resolveList)
	//{
	//	curl_slist_free_all(m_resolveList);
	//	m_resolveList = nullptr;
	//}

	//// =========================================================
	//// ② WinSock DNS → 生成 "域名:端口:IP"
	//// =========================================================
	//addrinfo* ai = nullptr;
	//int retDns = getaddrinfo(hostName.c_str(), NULL, NULL, &ai);

	//if (retDns == 0 && ai)
	//{
	//	sockaddr_in* addr = (sockaddr_in*)ai->ai_addr;

	//	char ip[64] = { 0 };
	//	inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

	//	char resolveEntry[256];
	//	sprintf(resolveEntry, "%s:%d:%s", hostName.c_str(), port, ip);

	//	TRACEI("[CURL] RESOLVE inject = %s", resolveEntry);

	//	// 必须用成员变量，否则 curl_easy_reset 会把旧的清掉
	//	m_resolveList = curl_slist_append(m_resolveList, resolveEntry);

	//	freeaddrinfo(ai);
	//}
	//else
	//{
	//	TRACEI("[CURL] getaddrinfo failed, fallback to curl DNS");
	//}

	// =========================================================
	// ③ reset 清空旧配置
	// =========================================================
	curl_easy_reset(m_curl);

	// =========================================================
	// ④ 把 DNS 注入回 curl（关键）
	// =========================================================
	//if (m_resolveList)
	//{
	//	curl_easy_setopt(m_curl, CURLOPT_RESOLVE, m_resolveList);
	//}

	{
		TRACEI("[TEST DNS] === WinSock DNS Test in SendHttpRequest ===");

		addrinfo hints = {};
		addrinfo* res = nullptr;

		int r = getaddrinfo(hostName.c_str(), NULL, &hints, &res);

		if (r != 0)
		{
			TRACEE("[TEST DNS] getaddrinfo('%s') FAILED, ret=%d, desc=%s",
				hostName.c_str(), r, gai_strerrorA(r));
		}
		else
		{
			TRACEI("[TEST DNS] getaddrinfo('%s') SUCCESS", hostName.c_str());
			freeaddrinfo(res);
		}

		TRACEI("[TEST DNS] ============================================");
	}

	// 打印当前接口 hostName 的真实值
	TRACEI("[CURL] INPUT HOST = '%s'", hostName.c_str());

	if (!m_curl)
		return -1;

	m_cancelled = false;
	m_step = STEP_NONE;
	response.clear();
	vecCookies.clear();

	// ===== 禁用 HTTP/2 / HTTP/3 =====
	curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

	// ===== 禁用 libcurl 自动 Cookie 管理 =====
	curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, "");
	curl_easy_setopt(m_curl, CURLOPT_COOKIEJAR, "");
	curl_easy_setopt(m_curl, CURLOPT_COOKIELIST, "ALL");

	// ===== 禁用连接复用，强制每次新建 TCP连接 =====
	curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1L);
	curl_easy_setopt(m_curl, CURLOPT_FRESH_CONNECT, 1L);

	// ===== 移除 Expect: 100-continue =====
	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "Expect:");
	curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

	///////////////////////////////////////////////////////////////////////////
	// Debug 回调（捕获 DNS / TCP / TLS / SEND / RECV）
	///////////////////////////////////////////////////////////////////////////
	curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, DebugCallback);
	curl_easy_setopt(m_curl, CURLOPT_DEBUGDATA, this);

	///////////////////////////////////////////////////////////////////////////
	// URL 构建
	///////////////////////////////////////////////////////////////////////////
	std::string fixedUrl = url;

	// 如果 URL 不以 "/" 开头，自动补一个
	if (!fixedUrl.empty() && fixedUrl[0] != '/')
		fixedUrl = "/" + fixedUrl;

	std::string fullUrl = "https://" + hostName + fixedUrl;

	curl_easy_setopt(m_curl, CURLOPT_URL, fullUrl.c_str());
	TRACEI("[CURL] FULL URL = %s", fullUrl.c_str());

	// 强制使用 IPv4（重要）
	curl_easy_setopt(m_curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
	// 禁用 DNS 缓存（重要）
	curl_easy_setopt(m_curl, CURLOPT_DNS_CACHE_TIMEOUT, 0L);
	// 不要全局 DNS 缓存
	curl_easy_setopt(m_curl, CURLOPT_DNS_USE_GLOBAL_CACHE, 0L);
	// 防止超时信号干扰 DNS
	curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);

	///////////////////////////////////////////////////////////////////////////
	// User-Agent 是一个运行在 Windows 10/11 64bit 上的 IE11（Trident 7.0 内核），并且行为兼容 Gecko 浏览器。
	///////////////////////////////////////////////////////////////////////////
	curl_easy_setopt(m_curl, CURLOPT_USERAGENT,
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64; Trident/7.0; rv:11.0) like Gecko sdologin-Launcher");

	///////////////////////////////////////////////////////////////////////////
	// Timeout
	///////////////////////////////////////////////////////////////////////////
	curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, timeout);
	curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, timeout);

	///////////////////////////////////////////////////////////////////////////
	// Proxy
	///////////////////////////////////////////////////////////////////////////
	if (proxyEnable)
	{
		// 自动读取系统代理
		curl_easy_setopt(m_curl, CURLOPT_PROXY, NULL);
	}
	else
	{
		// 禁用代理
		curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
		curl_easy_setopt(m_curl, CURLOPT_NOPROXY, "*");
	}

	///////////////////////////////////////////////////////////////////////////
	// HTTPS
	///////////////////////////////////////////////////////////////////////////
	// 统一按 HTTPS 处理
	//
	// HTTPS 证书验证设置
	//
	if (1)
	{
		// 抓包模式 / 开发模式
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
		TRACEI("[CURL] HTTPS verify = OFF");
	}
	else
	{
		// 正常生产环境
		curl_easy_setopt(m_curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 2L);

		char exePath[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, exePath, MAX_PATH);
		char* lastSlash = strrchr(exePath, '\\');
		if (lastSlash) *lastSlash = '\0';

		std::string caFile = std::string(exePath) + "\\cacert.pem";
		curl_easy_setopt(m_curl, CURLOPT_CAINFO, caFile.c_str());

		TRACEI("[CURL] HTTPS verify = ON, CA = %s", caFile.c_str());
	}

	///////////////////////////////////////////////////////////////////////////
	// 回调注册
	///////////////////////////////////////////////////////////////////////////
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);

	curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
	curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &vecCookies);

	curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
	curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, this);

	///////////////////////////////////////////////////////////////////////////
	// HTTP 方法
	///////////////////////////////////////////////////////////////////////////
	m_step = STEP_SEND; // 所有请求行为均视为 SEND 阶段开始

	if (_stricmp(method.c_str(), "POST") == 0)
	{
		curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, postData.size());
	}
	else if (_stricmp(method.c_str(), "GET") == 0)
	{
		curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
	}
	else
	{
		curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, method.c_str());
		if (!postData.empty())
		{
			curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
			curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, postData.size());
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// 执行请求
	///////////////////////////////////////////////////////////////////////////
	CURLcode res = curl_easy_perform(m_curl);

	///////////////////////////////////////////////////////////////////////////
	// Cancel
	///////////////////////////////////////////////////////////////////////////
	if (m_cancelled)
	{
		TRACEE(L"[CURL] STEP_SEND failed,cancelled");
		return -6;   // WinInet: cancel
	}

	///////////////////////////////////////////////////////////////////////////
	// Error classification  (-1 ~ -7)
	///////////////////////////////////////////////////////////////////////////
	if (res != CURLE_OK)
	{
		std::string step = StepToString(m_step);

		// 从映射表中查找 curl 错误描述
		auto it = g_curlErrMap.find(res);
		const char* errName = (it != g_curlErrMap.end() ? it->second.name.c_str() : "UNKNOWN");
		const char* errDesc = (it != g_curlErrMap.end() ? it->second.desc.c_str() : "Unknown error");

		// ---- DNS ----
		if (res == CURLE_COULDNT_RESOLVE_HOST)
		{
			TRACEE(L"[CURL] STEP_DNS failed: code=%d (%S) desc=%S",
				res, errName, errDesc);
			return -1;
		}

		// ---- TCP ----
		if (res == CURLE_COULDNT_CONNECT)
		{
			TRACEE(L"[CURL] STEP_TCP failed: code=%d (%S) desc=%S",
				res, errName, errDesc);
			return -1;
		}

		// ---- TLS ----
		if (res == CURLE_SSL_CONNECT_ERROR || res == CURLE_PEER_FAILED_VERIFICATION)
		{
			TRACEE(L"[CURL] STEP_TLS failed: code=%d (%S) desc=%S",
				res, errName, errDesc);
			return -1;
		}

		// ---- SEND ----
		if (res == CURLE_SEND_ERROR || res == CURLE_UPLOAD_FAILED)
		{
			TRACEE(L"[CURL] STEP_SEND failed: code=%d (%S) desc=%S",
				res, errName, errDesc);
			return -3;
		}

		// ---- RECV ----
		if (res == CURLE_RECV_ERROR)
		{
			TRACEE(L"[CURL] STEP_RECV failed: code=%d (%S) desc=%S",
				res, errName, errDesc);
			return -4;
		}

		// ---- TIMEOUT ----
		if (res == CURLE_OPERATION_TIMEDOUT)
		{
			TRACEE(L"[CURL] %S timeout: code=%d (%S) desc=%S",
				step.c_str(), res, errName, errDesc);
			return -5;
		}

		// ---- UNKNOWN ----
		TRACEE(L"[CURL] Unknown error at %S: code=%d (%S) desc=%S",
			step.c_str(), res, errName, errDesc);

		return -1;
	}

	///////////////////////////////////////////////////////////////////////////
	// HTTP 状态码
	///////////////////////////////////////////////////////////////////////////
	long httpCode = 0;
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);

	if (httpCode != 200)
	{
		TRACEE(L"[CURL] STEP_RECV failed,httpCode=%d", httpCode);
		return -6;     // WinInet: 非 200 → -6
	}

	///////////////////////////////////////////////////////////////////////////
	// DONE
	///////////////////////////////////////////////////////////////////////////
	m_step = STEP_DONE;
	TRACEI("[CURL] STEP_DONE,Request OK");

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// SendHttpRequest —— 核心实现
///////////////////////////////////////////////////////////////////////////////

int CurlHttpClient::SendHttpRequest(
	const string& hostName,
	int port,
	const string& url,
	const string& method,
	const string& postData,
	int timeout,
	string& response,
	vector<string>& vecCookies,
	bool proxyEnable,
	std::map<std::string, std::string>& headers,
	int requestCode)
{
	// =========================================================
	// ① 清空旧的 resolve list（一定要做）
	// =========================================================
	//if (m_resolveList)
	//{
	//	curl_slist_free_all(m_resolveList);
	//	m_resolveList = nullptr;
	//}

	//// =========================================================
	//// ② WinSock DNS → 生成 "域名:端口:IP"
	//// =========================================================
	//addrinfo* ai = nullptr;
	//int retDns = getaddrinfo(hostName.c_str(), NULL, NULL, &ai);

	//if (retDns == 0 && ai)
	//{
	//	sockaddr_in* addr = (sockaddr_in*)ai->ai_addr;

	//	char ip[64] = { 0 };
	//	inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

	//	char resolveEntry[256];
	//	sprintf(resolveEntry, "%s:%d:%s", hostName.c_str(), port, ip);

	//	TRACEI("[CURL] RESOLVE inject = %s", resolveEntry);

	//	// 必须用成员变量，否则 curl_easy_reset 会把旧的清掉
	//	m_resolveList = curl_slist_append(m_resolveList, resolveEntry);

	//	freeaddrinfo(ai);
	//}
	//else
	//{
	//	TRACEI("[CURL] getaddrinfo failed, fallback to curl DNS");
	//}

	// =========================================================
	// ③ reset 清空旧配置
	// =========================================================
	curl_easy_reset(m_curl);

	// =========================================================
	// ④ 把 DNS 注入回 curl（关键）
	// =========================================================
	//if (m_resolveList)
	//{
	//	curl_easy_setopt(m_curl, CURLOPT_RESOLVE, m_resolveList);
	//}

	{
		TRACEI("[TEST DNS] === WinSock DNS Test in SendHttpRequest ===");

		addrinfo hints = {};
		addrinfo* res = nullptr;

		int r = getaddrinfo(hostName.c_str(), NULL, &hints, &res);

		if (r != 0)
		{
			TRACEE("[TEST DNS] getaddrinfo('%s') FAILED, ret=%d, desc=%S",
				hostName.c_str(), r, gai_strerrorA(r));
		}
		else
		{
			TRACEI("[TEST DNS] getaddrinfo('%s') SUCCESS", hostName.c_str());
			freeaddrinfo(res);
		}

		TRACEI("[TEST DNS] ============================================");
	}

	// 【必须加在这里】打印当前接口 hostName 的真实值
	TRACEI("[CURL] INPUT HOST = '%s'", hostName.c_str());

	//所有需要用到的对象提前声明
	std::string fixedUrl = url;
	std::string fullUrl;

	int result = 0;  // 默认成功

	// 根据端口决定协议
	std::string scheme;

	if (!m_curl)
	{
		result = -1;
		goto cleanup;
	}

	m_cancelled = false;
	m_step = STEP_NONE;
	response.clear();
	vecCookies.clear();

	// ===== 禁用 HTTP/2 / HTTP/3 =====
	curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

	// ===== 禁用 libcurl 自动 Cookie 管理 =====
	curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, "");
	curl_easy_setopt(m_curl, CURLOPT_COOKIEJAR, "");
	curl_easy_setopt(m_curl, CURLOPT_COOKIELIST, "ALL");

	// ===== 禁用连接复用，强制每次新建 TCP连接 =====
	curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1L);
	curl_easy_setopt(m_curl, CURLOPT_FRESH_CONNECT, 1L);

	// ===== 移除 Expect: 100-continue =====
	//struct curl_slist* headerList = NULL;
	//headerList = curl_slist_append(headerList, "Expect:");
	//curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headerList);

	///////////////////////////////////////////////////////////////////////////
	// HTTP Header 构建（支持 Gunion 自定义 Header）
	///////////////////////////////////////////////////////////////////////////
	struct curl_slist* headerList = NULL;
	// ① 移除 Expect: 100-continue
	headerList = curl_slist_append(headerList, "Expect:");
	// ② 如果是 Gunion 请求（headers 非空）
	for (const auto& kv : headers)
	{
		std::string line = kv.first + ": " + kv.second;
		headerList = curl_slist_append(headerList, line.c_str());

		TRACEI("[CURL] ADD HEADER: %s", line.c_str());
	}

	// ③ 设置到 curl
	curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headerList);

	///////////////////////////////////////////////////////////////////////////
	// Debug 回调（捕获 DNS / TCP / TLS / SEND / RECV）
	///////////////////////////////////////////////////////////////////////////
	curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, DebugCallback);
	curl_easy_setopt(m_curl, CURLOPT_DEBUGDATA, this);

	///////////////////////////////////////////////////////////////////////////
	// URL 构建
	///////////////////////////////////////////////////////////////////////////

	// 如果 URL 不以 "/" 开头，自动补一个
	// 如果 URL 不以 "/" 开头，自动补一个
	if (!fixedUrl.empty() && fixedUrl[0] != '/')
		fixedUrl = "/" + fixedUrl;

	if (requestCode == 72 || requestCode == 97 || requestCode == 96)
		scheme = "https";
	else if (port == INTERNET_DEFAULT_HTTPS_PORT)
		scheme = "https";
	else
		scheme = "http";

	// 构建完整 URL
	fullUrl = scheme + "://" + hostName + fixedUrl;

	// 设置 curl URL
	curl_easy_setopt(m_curl, CURLOPT_URL, fullUrl.c_str());

	// 打印日志
	TRACEI("[CURL] FULL URL = %s", fullUrl.c_str());

	// 强制使用 IPv4（重要）
	curl_easy_setopt(m_curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
	// 禁用 DNS 缓存（重要）
	curl_easy_setopt(m_curl, CURLOPT_DNS_CACHE_TIMEOUT, 0L);
	// 不要全局 DNS 缓存
	curl_easy_setopt(m_curl, CURLOPT_DNS_USE_GLOBAL_CACHE, 0L);
	// 防止超时信号干扰 DNS
	curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);

	///////////////////////////////////////////////////////////////////////////
	// User-Agent 是一个运行在 Windows 10/11 64bit 上的 IE11（Trident 7.0 内核），并且行为兼容 Gecko 浏览器。
	///////////////////////////////////////////////////////////////////////////
	curl_easy_setopt(m_curl, CURLOPT_USERAGENT,
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64; Trident/7.0; rv:11.0) like Gecko sdologin-Launcher");

	///////////////////////////////////////////////////////////////////////////
	// Timeout
	///////////////////////////////////////////////////////////////////////////
	curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, timeout);
	curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, timeout);

	///////////////////////////////////////////////////////////////////////////
	// Proxy
	///////////////////////////////////////////////////////////////////////////
	// -----------------------------------------------------------------------------
	// Proxy 设置
	//
	// proxyEnable = true
	//      表示允许使用系统代理。
	//      libcurl 不会像 WinInet 一样自动读取 Windows 系统代理配置，
	//      CURLOPT_PROXY = NULL 的含义是：不显式指定代理地址，让 libcurl
	//      按默认方式处理（例如读取环境变量 HTTP_PROXY / HTTPS_PROXY）。
	//
	// proxyEnable = false
	//      表示强制禁用代理，所有请求直接连接服务器。
	//      这样可以避免用户本地代理/VPN/加速器/PAC 等网络环境对登录接口
	//      的影响，例如：
	//          - VPN / Clash / 公司代理导致接口访问失败
	//          - HTTPS 被代理劫持
	//          - DNS 被污染
	//          - 登录请求被错误转发
	//
	//      很多游戏 Launcher / 登录 SDK 都会默认关闭代理，保证登录网络
	//      请求的稳定性和可控性。
	//
	// 注意：
	//      CURLOPT_NOPROXY = "*" 表示所有域名都不允许走代理。
	// -----------------------------------------------------------------------------
	if (proxyEnable)
	{
		// 允许使用代理（不主动指定代理地址）
		// 如果系统或环境变量配置了代理，libcurl 可能会使用这些代理。
		curl_easy_setopt(m_curl, CURLOPT_PROXY, NULL);
	}
	else
	{
		// 禁用代理，强制所有请求直连服务器
		curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
		// "*" 表示所有域名都不使用代理
		curl_easy_setopt(m_curl, CURLOPT_NOPROXY, "*");

		//curl_easy_setopt(m_curl, CURLOPT_PROXY, "127.0.0.1:8888");
		//curl_easy_setopt(m_curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
	}

	///////////////////////////////////////////////////////////////////////////
	// HTTPS
	///////////////////////////////////////////////////////////////////////////
	// 统一按 HTTPS 处理
	//
	// HTTPS 证书验证设置
	//
	if (1)
	{
		// 抓包模式 / 开发模式
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
		TRACEI("[CURL] HTTPS verify = OFF");
	}
	else
	{
		// 正常生产环境
		curl_easy_setopt(m_curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 2L);

		char exePath[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, exePath, MAX_PATH);
		char* lastSlash = strrchr(exePath, '\\');
		if (lastSlash) *lastSlash = '\0';

		std::string caFile = std::string(exePath) + "\\cacert.pem";
		curl_easy_setopt(m_curl, CURLOPT_CAINFO, caFile.c_str());

		TRACEI("[CURL] HTTPS verify = ON, CA = %s", caFile.c_str());
	}

	///////////////////////////////////////////////////////////////////////////
	// 回调注册
	///////////////////////////////////////////////////////////////////////////
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);

	curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
	curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &vecCookies);

	curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
	curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, this);

	///////////////////////////////////////////////////////////////////////////
	// HTTP 方法
	///////////////////////////////////////////////////////////////////////////
	m_step = STEP_SEND; // 所有请求行为均视为 SEND 阶段开始

	if (_stricmp(method.c_str(), "POST") == 0)
	{
		curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, postData.size());
	}
	else if (_stricmp(method.c_str(), "GET") == 0)
	{
		curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
	}
	else
	{
		curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, method.c_str());
		if (!postData.empty())
		{
			curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
			curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, postData.size());
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// 执行请求
	///////////////////////////////////////////////////////////////////////////
	CURLcode res = curl_easy_perform(m_curl);

	///////////////////////////////////////////////////////////////////////////
	// Cancel
	///////////////////////////////////////////////////////////////////////////
	if (m_cancelled)
	{
		TRACEE(L"[CURL] STEP_SEND failed,cancelled");
		result = -6;
		goto cleanup;   // WinInet: cancel
	}

	///////////////////////////////////////////////////////////////////////////
	// Error classification  (-1 ~ -7)
	///////////////////////////////////////////////////////////////////////////
	if (res != CURLE_OK)
	{
		std::string step = StepToString(m_step);

		// 从映射表中查找 curl 错误描述
		auto it = g_curlErrMap.find(res);
		const char* errName = (it != g_curlErrMap.end() ? it->second.name.c_str() : "UNKNOWN");
		const char* errDesc = (it != g_curlErrMap.end() ? it->second.desc.c_str() : "Unknown error");

		// ---- DNS ----
		if (res == CURLE_COULDNT_RESOLVE_HOST)
		{
			TRACEE(L"[CURL] STEP_DNS failed: code=%d (%S) desc=%S",
				res, errName, errDesc);
			result = -1;
			goto cleanup;
		}

		// ---- TCP ----
		if (res == CURLE_COULDNT_CONNECT)
		{
			TRACEE(L"[CURL] STEP_TCP failed: code=%d (%S) desc=%S",
				res, errName, errDesc);
			result = -1;
			goto cleanup;
		}

		// ---- TLS ----
		if (res == CURLE_SSL_CONNECT_ERROR || res == CURLE_PEER_FAILED_VERIFICATION)
		{
			TRACEE(L"[CURL] STEP_TLS failed: code=%d (%S) desc=%S",
				res, errName, errDesc);
			result = -1;
			goto cleanup;
		}

		// ---- SEND ----
		if (res == CURLE_SEND_ERROR || res == CURLE_UPLOAD_FAILED)
		{
			TRACEE(L"[CURL] STEP_SEND failed: code=%d (%S) desc=%S",
				res, errName, errDesc);
			result = -3;
			goto cleanup;
		}

		// ---- RECV ----
		if (res == CURLE_RECV_ERROR)
		{
			TRACEE(L"[CURL] STEP_RECV failed: code=%d (%S) desc=%S",
				res, errName, errDesc);
			result = -4;
			goto cleanup;
		}

		// ---- TIMEOUT ----
		if (res == CURLE_OPERATION_TIMEDOUT)
		{
			TRACEE(L"[CURL] %S timeout: code=%d (%S) desc=%S",
				step.c_str(), res, errName, errDesc);
			result = -5;
			goto cleanup;
		}

		// ---- UNKNOWN ----
		TRACEE(L"[CURL] Unknown error at %S: code=%d (%S) desc=%S",
			step.c_str(), res, errName, errDesc);

		result = -1;
		goto cleanup;
	}

	///////////////////////////////////////////////////////////////////////////
	// HTTP 状态码
	///////////////////////////////////////////////////////////////////////////
	long httpCode = 0;
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);

	if (httpCode != 200)
	{
		TRACEE(L"[CURL] STEP_RECV failed,httpCode=%d", httpCode);
		result = -6;     // WinInet: 非 200 → -6
		goto cleanup;
	}

	///////////////////////////////////////////////////////////////////////////
	// DONE
	///////////////////////////////////////////////////////////////////////////
	m_step = STEP_DONE;
	TRACEI("[CURL] STEP_DONE,Request OK");

cleanup:

	if (headerList)
	{
		curl_slist_free_all(headerList);
		headerList = NULL;
	}

	return result;
}
//
