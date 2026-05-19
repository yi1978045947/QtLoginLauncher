#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <curl/curl.h>
#include <map>

using std::string;
using std::vector;

/**
 * CurlHttpClient
 * ------------------------------------------------------------
 * 使用 libcurl + OpenSSL 实现的跨平台 HTTP 客户端。
 * 功能目标：
 *   - 完全替代 WinInet，不依赖系统 TLS 版本（TLS1.0/1.1/1.2/1.3 不受系统开关影响）
 *   - 默认使用 OpenSSL 的加密能力，避免 Windows GPO / 防火墙 / IE 配置造成的干扰
 *   - 与原 HttpClient API 完全一致，方便无缝替换 HttpThread
 *   - 支持 Cancel 中断（通过 libcurl XFERINFO 回调）
 *   - 支持 cookies、代理、超时、自定义请求方法等
 *
 * 典型用途：
 *   用于登录器 / SDK 内的所有 HTTP 请求（获取公钥、登录、短信、票据等）
 */
class CurlHttpClient
{
public:
	struct CurlErrInfo {
		std::string name;   // 枚举名字，如 "CURLE_COULDNT_CONNECT"
		std::string desc;   // 英文描述，如 "Failed to connect to server"
	};
	static const std::map<int, CurlErrInfo> g_curlErrMap;

public:

	/**
	 * 与 WinInet 一样的 “步骤阶段”
	 * ------------------------------------------------------------
	 * 用于打印 WinInet 风格的调试日志，例如：
	 *
	 *   [CURL][DNS] Resolving DNS...
	 *   [CURL][TCP_CONNECT] TCP connected.
	 *   [CURL][TLS_HANDSHAKE] TLS handshake OK
	 *   [CURL][SEND] Sending data...
	 *   [CURL][RECV] Receiving body...
	 *
	 * 便于调试：到底在哪一步失败？
	 */
	enum CurlStep {
		STEP_NONE = 0,
		STEP_DNS,
		STEP_TCP,
		STEP_TLS,
		STEP_SEND,
		STEP_RECV,
		STEP_DONE
	};

public:
	CurlHttpClient();
	~CurlHttpClient();

	/**
	 * SendHttpRequest
	 * ------------------------------------------------------------
	 * 统一的 HTTP 请求接口，与原 HttpClient 保持一致。
	 *
	 * @param hostName      - 服务器域名，例如 "login.sdo.com"
	 * @param port          - 端口号(80 / 443)，自动拼接到 URL 中
	 * @param url           - 请求路径，例如 "/auth/getPublicKey"
	 * @param method        - 请求方法："GET" / "POST" / "PUT" / "DELETE"
	 * @param postData      - POST / PUT 的请求 body；GET 时忽略
	 * @param timeout       - 超时时间（毫秒）
	 * @param response      - 输出：服务器响应内容
	 * @param vecCookies    - 输出：服务器下发的 "Set-Cookie" 列表
	 * @param proxyEnable   - 是否启用系统代理（true=启用，false=直连）
	 *
	 * @return
	 *   0     - 请求成功
	 *
	 * 下面为增强后的 curl 精细错误码：
	 *  -1     - curl handle 创建异常 / 未初始化
	 *  -2     - DNS 解析失败（CURLE_COULDNT_RESOLVE_HOST）
	 *  -3     - TCP 连接失败（CURLE_COULDNT_CONNECT）
	 *  -4     - TLS 握手失败（证书问题 / 握手不兼容）
	 *  -5     - 发送失败（write error）
	 *  -6     - 接收失败（read error）
	 *  -7     - 超时（连接/传输）
	 *  -8     - HTTP 状态码 != 200
	 *  -9     - 用户主动 Cancel()
	 * -10     - 代理失败（CURLE_COULDNT_RESOLVE_PROXY）
	 * -11     - 未分类 curl 错误
	 */
	int SendHttpRequest(
		const string& hostName,
		int port,
		const string& url,
		const string& method,
		const string& postData,
		int timeout,
		string& response,
		vector<string>& vecCookies,
		bool proxyEnable);

	/**
	 * SendHttpRequest(Gunion专用）
	 * ------------------------------------------------------------
	 * 统一的 HTTP 请求接口，与原 HttpClient 保持一致。
	 *
	 * @param hostName      - 服务器域名，例如 "login.sdo.com"
	 * @param port          - 端口号(80 / 443)，自动拼接到 URL 中
	 * @param url           - 请求路径，例如 "/auth/getPublicKey"
	 * @param method        - 请求方法："GET" / "POST" / "PUT" / "DELETE"
	 * @param postData      - POST / PUT 的请求 body；GET 时忽略
	 * @param timeout       - 超时时间（毫秒）
	 * @param response      - 输出：服务器响应内容
	 * @param vecCookies    - 输出：服务器下发的 "Set-Cookie" 列表
	 * @param proxyEnable   - 是否启用系统代理（true=启用，false=直连）
	 * @param headers       - Gunion关于post接口的请求头部
	 * @param requestCode   - Gunion关于post接口的请求code
	 *
	 * @return
	 *   0     - 请求成功
	 *
	 * 下面为增强后的 curl 精细错误码：
	 *  -1     - curl handle 创建异常 / 未初始化
	 *  -2     - DNS 解析失败（CURLE_COULDNT_RESOLVE_HOST）
	 *  -3     - TCP 连接失败（CURLE_COULDNT_CONNECT）
	 *  -4     - TLS 握手失败（证书问题 / 握手不兼容）
	 *  -5     - 发送失败（write error）
	 *  -6     - 接收失败（read error）
	 *  -7     - 超时（连接/传输）
	 *  -8     - HTTP 状态码 != 200
	 *  -9     - 用户主动 Cancel()
	 * -10     - 代理失败（CURLE_COULDNT_RESOLVE_PROXY）
	 * -11     - 未分类 curl 错误
	 */
	int SendHttpRequest(
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
		int requestCode);

	/**
	 * Cancel
	 * ------------------------------------------------------------
	 * 设置取消标志，正在执行的 curl_easy_perform 将会被中断。
	 *
	 * 注意：
	 *   - Cancel 是异步的，但会在 progress 回调中立即停止传输
	 *   - 返回值目前无意义，可统一返回 0
	 */
	int Cancel();

	/**
	 * SetVerifyCert
	 * ------------------------------------------------------------
	 * 设置 HTTPS 是否启用证书校验（生产 / 调试环境切换功能）。
	 *
	 * 参数：
	 *   enable = true
	 *       开启证书校验（生产环境）
	 *       - 启用 SSL_VERIFYPEER = 1（验证服务器证书）
	 *       - 启用 SSL_VERIFYHOST = 2（验证域名）
	 *       - 必须加载 CA 证书（cacert.pem）
	 *
	 *   enable = false
	 *       关闭证书校验（调试 / 抓包环境）
	 *       - 可配合 Fiddler / Charles / HttpAnalyzer 等代理抓包
	 *       - 不验证服务器证书或域名
	 *       - 避免抓包导致的 CURLE_PEER_FAILED_VERIFICATION(60)
	 *
	 * 使用场景：
	 *   - 调试时关闭（抓包）
	 *   - 正式环境必须开启（安全）
	 *
	 * 默认值：
	 *   m_verifyCert = true（开启校验）
	 */
	void SetVerifyCert(bool enable);
private:
	/**
	 * WriteCallback
	 * ------------------------------------------------------------
	 * libcurl 写回调，用于接收响应 body。
	 * 所有内容被附加到 response（string）中。
	 */
	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

	/**
	 * HeaderCallback
	 * ------------------------------------------------------------
	 * libcurl 头部回调，用于解析服务器返回的 HTTP Header。
	 * 用它来解析 "Set-Cookie" 头部。
	 */
	static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata);

	/**
	 * ProgressCallback
	 * ------------------------------------------------------------
	 * libcurl 进度回调，每个数据块传输时都会被触发。
	 *
	 * 用该回调实现 Cancel：
	 *   - 如果 m_cancelled=true，则返回非 0，curl 会立刻终止请求。
	 */
	static int ProgressCallback(void* clientp,
		curl_off_t dltotal, curl_off_t dlnow,
		curl_off_t ultotal, curl_off_t ulnow);

	/**
	 * DebugCallback（WinInet风格步骤日志）
	 * ------------------------------------------------------------
	 * 解析 curl Verbose 信息，识别：
	 *   - DNS 解析
	 *   - TCP 连接
	 *   - TLS 握手
	 *   - 发送阶段
	 *   - 接收阶段
	 *
	 * 用于替代 WinInet 的 INTERNET_STATUS 回调。
	 */
	static int DebugCallback(CURL* handle, curl_infotype type,
		char* data, size_t size, void* userptr);

	/**
	 * 打印 WinInet 风格的 step 日志
	 */
	void LogStep(const char* fmt, ...);

	/**
	 * 将枚举的 Step 转换为字符串
	 */
	std::string StepToString(CurlStep step);

private:
	CURL* m_curl;                       ///< libcurl easy handle
	std::atomic<bool> m_cancelled;      ///< Cancel 标志，由 Cancel() 设置，由 ProgressCallback 检查

	CurlStep m_step = STEP_NONE;        ///< 当前请求所处阶段（用于调试日志）

	//struct curl_slist* m_resolveList = nullptr;   ///< 保存 IP 绑定列表（必须是成员变量）

	bool m_verifyCert;                 ///<  默认启用证书校验（生产环境）
};
/**
 * SendHttpRequest
 * ------------------------------------------------------------
 * 完整执行一次 HTTP 请求，与原 HttpClient 完全一致的 API 参数。
 *
 * 返回值说明（模拟 WinInet 旧错误码逻辑）：
 *   0   - 成功
 *  -1   - curl handle 创建异常
 *  -2   - curl_easy_perform 执行失败（包括网络错误）
 *  -3   - 由 Cancel() 主动中断
 *  -4   - HTTP 状态码 != 200
 *  -5   - 超时（CURLE_OPERATION_TIMEDOUT）
 */
 //int CurlHttpClient::SendHttpRequest(
 //    const string& hostName,
 //    int port,
 //    const string& url,
 //    const string& method,
 //    const string& postData,
 //    int timeout,
 //    string& response,
 //    vector<string>& vecCookies,
 //    bool proxyEnable)
 //{
 //    if (!m_curl)
 //        return -1;
 //
 //    ///////////////////////////////////////////////////////////////////////////
 //    // 初始化环境
 //    ///////////////////////////////////////////////////////////////////////////
 //
 //    m_cancelled = false;
 //    response.clear();
 //    vecCookies.clear();
 //
 //    curl_easy_reset(m_curl);
 //
 //    CURLcode res;
 //
 //    ///////////////////////////////////////////////////////////////////////////
 //    // 构建 URL
 //    ///////////////////////////////////////////////////////////////////////////
 //
 //    std::stringstream ss;
 //    ss << (port == 443 ? "https://" : "http://") << hostName;
 //
 //    if (port != 80 && port != 443)
 //        ss << ":" << port;
 //
 //    ss << url;
 //
 //    std::string fullUrl = ss.str();
 //
 //    curl_easy_setopt(m_curl, CURLOPT_URL, fullUrl.c_str());
 //
 //    ///////////////////////////////////////////////////////////////////////////
 //    // 设置 User-Agent（和原 WinInet 一致）
 //    ///////////////////////////////////////////////////////////////////////////
 //
 //    curl_easy_setopt(m_curl, CURLOPT_USERAGENT,
 //        "Mozilla/5.0 (Windows NT 10.0; Win64; x64; Trident/7.0; rv:11.0) like Gecko");
 //
 //    ///////////////////////////////////////////////////////////////////////////
 //    // 设置超时（毫秒）
 //    ///////////////////////////////////////////////////////////////////////////
 //
 //    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, timeout);
 //    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, timeout);
 //
 //    ///////////////////////////////////////////////////////////////////////////
 //    // 代理设置：模拟 WinInet 的行为
 //    ///////////////////////////////////////////////////////////////////////////
 //
 //    if (proxyEnable)
 //    {
 //        // 使用系统代理（空字符串表示自动读取系统代理设置）
 //        curl_easy_setopt(m_curl, CURLOPT_PROXY, "");
 //        curl_easy_setopt(m_curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
 //    }
 //    else
 //    {
 //        // 完全禁用代理
 //        curl_easy_setopt(m_curl, CURLOPT_NOPROXY, "*");
 //    }
 //
 //    ///////////////////////////////////////////////////////////////////////////
 //    // HTTPS 设置（使用 OpenSSL，不依赖系统）
 //    ///////////////////////////////////////////////////////////////////////////
 //
 //    if (port == 443)
 //    {
 //        curl_easy_setopt(m_curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
 //
 //        // 启用严格证书校验（业务要求可调整）
 //        curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
 //        curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 2L);
 //
 //        // ★ 若服务器证书有问题，可以放开以下两行（非推荐）
 //        // curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
 //        // curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
 //    }
 //
 //    ///////////////////////////////////////////////////////////////////////////
 //    // 注册 body 和 header 回调
 //    ///////////////////////////////////////////////////////////////////////////
 //
 //    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
 //    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
 //
 //    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
 //    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &vecCookies);
 //
 //    ///////////////////////////////////////////////////////////////////////////
 //    // 进度回调（用于 Cancel）
 //    ///////////////////////////////////////////////////////////////////////////
 //
 //    curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
 //    curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, this);
 //    curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);
 //
 //    ///////////////////////////////////////////////////////////////////////////
 //    // 设置请求方法
 //    ///////////////////////////////////////////////////////////////////////////
 //
 //    if (_stricmp(method.c_str(), "POST") == 0)
 //    {
 //        curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
 //        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
 //        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, postData.size());
 //    }
 //    else if (_stricmp(method.c_str(), "GET") == 0)
 //    {
 //        curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
 //    }
 //    else
 //    {
 //        // 自定义方法（如 PUT / DELETE）
 //        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, method.c_str());
 //
 //        if (!postData.empty())
 //        {
 //            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
 //            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, postData.size());
 //        }
 //    }
 //
 //    ///////////////////////////////////////////////////////////////////////////
 //    // 执行请求
 //    ///////////////////////////////////////////////////////////////////////////
 //
 //    res = curl_easy_perform(m_curl);
 //
 //    // 若被用户主动取消
 //    if (m_cancelled)
 //        return -3;
 //
 //    // curl 执行失败
 //    if (res != CURLE_OK)
 //    {
 //        if (res == CURLE_OPERATION_TIMEDOUT)
 //            return -5;       // 超时
 //
 //        return -2;           // 通用传输错误
 //    }
 //
 //    ///////////////////////////////////////////////////////////////////////////
 //    // 检查 HTTP 响应码
 //    ///////////////////////////////////////////////////////////////////////////
 //
 //    long httpCode = 0;
 //    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);
 //
 //    if (httpCode != 200)
 //        return -4;
 //
 //    return 0;
 //}