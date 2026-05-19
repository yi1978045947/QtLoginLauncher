#include "stdafx.h"
#include "CurlHttpDownload.h"
#include <curl/curl.h>
#include <vector>
#include <string>
#include <windows.h>
#include "Tracer.h"

// ============================================================================
// CurlStage —— Curl 网络调试阶段枚举
//
// 用于把 libcurl 的“事件流日志”整理成“阶段流日志”，
// 避免 STEP_TLS / STEP_CERT 重复刷屏，提升可读性。
//
// 阶段说明：
//   NONE : 尚未进入任何网络阶段
//   DNS  : 域名解析 / 尝试连接 IP
//   TCP  : TCP 连接建立成功
//   TLS  : TLS 握手进行中或完成
//   CERT : TLS 证书阶段（服务器证书已收到）
//   HTTP : HTTP 请求/响应阶段
// ============================================================================
enum class CurlStage
{
	NONE,
	DNS,
	TCP,
	TLS,
	CERT,
	HTTP
};

// ============================================================================
// LogCurlStage
//
// 用途：
//   仅在 Curl 网络阶段发生变化时输出日志，
//   避免同一阶段（如 TLS / CERT）被重复打印，
//   让日志更符合“人类阅读习惯”。
//
// 注意：
//   - 这是“日志层工具”，不影响任何 curl 行为
//   - 不参与网络逻辑，仅用于调试可读性
// ============================================================================
static CurlStage g_lastCurlStage = CurlStage::NONE;

static void LogCurlStage(CurlStage stage, const wchar_t* desc)
{
	if (g_lastCurlStage != stage)
	{
		g_lastCurlStage = stage;
		TRACEI("[CURL-DL] %s", desc);
	}
}

// ============================================================================
// WriteMemoryCallback
// libcurl 收到数据后调用此回调函数，把内容写入 std::vector<unsigned char>
// 每次收到一块数据，都会 append 到 vector 尾部。
// ============================================================================
static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t total = size * nmemb;

	// userp 是调用方传入的 std::vector<unsigned char>
	auto* buffer = (std::vector<unsigned char>*)userp;

	// 把收到的数据块追加到 vector 尾部
	buffer->insert(buffer->end(),
		(unsigned char*)contents,
		(unsigned char*)contents + total);

	return total;
}

// ============================================================================
// CurlDebugCallback —— 完整调试回调
// libcurl 的 verbose 模式会在内部调用此回调，全量输出内部行为：
//   * TEXT：常规日志
//   * HEADER_IN：服务器响应头
//   * HEADER_OUT：上传头部
//   * DATA_IN：收到的数据字节数（不打印二进制）
//   * DATA_OUT：发送的数据字节数
// 用法与 CurlHttpClient 完全一致，保证日志风格统一。
// ============================================================================
static int CurlDebugCallback(
	CURL* handle,
	curl_infotype type,
	char* data,
	size_t size,
	void* userptr)
{
	// =========================
	// 1. curl 内部状态（只解析，不打印）
	// =========================
	if (type == CURLINFO_TEXT && data && size > 0)
	{
		std::string text(data, size);

		// DNS
		if (text.find("Trying ") != std::string::npos)
		{
			// 进入 DNS / 连接尝试阶段
			LogCurlStage(CurlStage::DNS, L"DNS: resolving & trying IP");
		}

		// TCP
		else if (text.find("Connected to") != std::string::npos)
		{
			// TCP 连接已建立
			LogCurlStage(CurlStage::TCP, L"TCP: connected");
		}

		// TLS 阶段文本
		else if (text.find("TLS handshake") != std::string::npos ||
			text.find("SSL connection using") != std::string::npos)
		{
			// TLS 握手进行中或已完成
			LogCurlStage(CurlStage::TLS, L"TLS: handshake in progress");
		}

		// 证书阶段（只打标记）
		else if (text.find("Certificate") != std::string::npos ||
			text.find("Server certificate") != std::string::npos)
		{
			// TLS 服务器证书已接收
			LogCurlStage(CurlStage::CERT, L"TLS: server certificate received");
		}

		// 重要：不打印任何 TEXT 原文
		return 0;
	}

	// =========================
	// 2. HTTP Header（安全文本）
	// =========================
	if (type == CURLINFO_HEADER_OUT)
	{
		// 进入 HTTP 请求阶段
		LogCurlStage(CurlStage::HTTP, L"HTTP: request sent");
		TRACEI("[CURL-DL] => HTTP Header: %.*hs", (int)size, data);
		return 0;
	}

	if (type == CURLINFO_HEADER_IN)
	{
		TRACEI("[CURL-DL] <= HTTP Header: %.*hs", (int)size, data);
		return 0;
	}

	// =========================
	// 3. HTTP Body（只看大小）
	// =========================
	if (type == CURLINFO_DATA_OUT)
	{
		TRACEI("[CURL-DL] => HTTP Body %d bytes", (int)size);
		return 0;
	}

	if (type == CURLINFO_DATA_IN)
	{
		TRACEI("[CURL-DL] <= HTTP Body %d bytes", (int)size);
		return 0;
	}

	// =========================
	// 4. TLS 二进制数据
	//
	// 说明：
	//   TLS binary 数据属于加密后的底层传输内容，
	//   对当前阶段化调试（DNS / TCP / TLS / HTTP）没有直接帮助。
	//   为了避免日志噪音，这里默认不打印任何 TLS binary 大小信息。
	//
	//   如需排查 TLS 分包 / MTU / 异常拆包问题，可临时恢复日志。
	// =========================
	if (type == CURLINFO_SSL_DATA_OUT)
	{
		// TLS 加密数据（发送），默认不打印
		return 0;
	}

	if (type == CURLINFO_SSL_DATA_IN)
	{
		// TLS 加密数据（接收），默认不打印
		return 0;
	}

	return 0;
}

// ============================================================================
// DownloadToMemory
// 使用 libcurl 下载一个 URL，并把内容放入 outData 内存中。
// 必须说明的是：
//   全程等价于 CurlHttpClient 网络层参数（HTTP1.1、IPv4、No Cache）
//   始终开启 verbose（方便定位验证码加载失败问题）
//   支持关闭证书 verify（verifyCert=false）
// ============================================================================

bool CurlHttpDownload::DownloadToMemory(
	const std::string& url,
	std::vector<unsigned char>& outData,
	long timeoutMs,
	bool verifyCert
)
{
	// 每次新的下载开始时，重置 Curl 调试阶段
	g_lastCurlStage = CurlStage::NONE;

	// 创建 easy 句柄
	CURL* curl = curl_easy_init();
	if (!curl)
		return false;

	outData.clear();

	TRACEI("[CURL-DL] Start Download: %S", url.c_str());

	// ========================================================================
	// 基础配置：URL
	// ========================================================================
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	// ========================================================================
	// HTTP 协议行为设置（与 CurlHttpClient 完全保持一致）
	// ========================================================================

	// 强制使用 HTTP/1.1 —— 兼容性最好，避免 HTTP/2 导致的不兼容情况
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

	// 禁用连接复用，确保每次重新建立 TCP（避免 Keep-Alive 造成不可控连接重用）
	curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
	curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1L);

	// 禁用 "Expect: 100-continue"，减少 1 RTT 延迟
	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "Expect:");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	// 超时设置（毫秒）
	curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeoutMs);

	// 强制只用 IPv4，避免某些网络的 IPv6 解析失败导致超时
	curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

	// 禁用 DNS 缓存，保证每次都重新解析（对诊断 DNS 问题特别关键）
	curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 0L);
	curl_easy_setopt(curl, CURLOPT_DNS_USE_GLOBAL_CACHE, 0L);

	// 禁用 signal，避免超时导致 SIGPIPE
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	// User-Agent 与 CurlHttpClient 保持完全一致
	curl_easy_setopt(curl, CURLOPT_USERAGENT,
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64; Trident/7.0; rv:11.0) like Gecko sdologin-Launcher");

	// ========================================================================
	// 数据写入回调（把下载内容写入 vector）
	// ========================================================================
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outData);

	// ========================================================================
	// HTTPS 证书校验开关
	// verifyCert = false 时，关闭校验（类似 Fiddler 抓包模式）
	// ========================================================================
	if (!verifyCert)
	{
		TRACEI("[CURL-DL] HTTPS Verify OFF");

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	}
	else
	{
		TRACEI("[CURL-DL] HTTPS Verify ON");

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

		// CA 文件与 CurlHttpClient 使用同一个 cacert.pem
		char exePath[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, exePath, MAX_PATH);
		char* lastSlash = strrchr(exePath, '\\');
		if (lastSlash) *lastSlash = '\0';

		std::string caPath = std::string(exePath) + "\\cacert.pem";
		curl_easy_setopt(curl, CURLOPT_CAINFO, caPath.c_str());

		TRACEI("[CURL-DL] CA file = %s", caPath.c_str());
	}

	// ========================================================================
	// 开启 verbose 并绑定 DebugCallback
	// 这保证能看到：DNS / TCP / TLS / Header / Body 的全部过程
	// ========================================================================
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, CurlDebugCallback);

	// ========================================================================
	// 执行下载
	// ========================================================================
	CURLcode res = curl_easy_perform(curl);

	// 必须释放 header 列表
	curl_slist_free_all(headers);

	// 释放 easy 句柄
	curl_easy_cleanup(curl);

	if (res != CURLE_OK)
	{
		TRACEE("[CURL-DL] Download FAILED: code=%d (%S)",
			res, curl_easy_strerror(res));
		return false;
	}

	TRACEI("[CURL-DL] Download SUCCESS, size=%d bytes", outData.size());
	return true;
}
