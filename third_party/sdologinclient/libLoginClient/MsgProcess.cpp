#include "StdAfx.h"
#include "MsgProcess.h"
#include <libUtil/json/json.h>
#include <WinSock2.h>
#include "SdoBaseClient.h"
#include "CommonFun.h"
#include "UrlCode.h"
#include <libUtil/md5.h>
#include <Iphlpapi.h>
#include <string>
#include "ComputerInfo.h"
#include "LoginClient.h"
#include <sstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <windows.h> 
#include "Tracer.h"
#include <algorithm>

#pragma comment(lib,"Iphlpapi.lib")

#pragma warning (disable:4200) 
#pragma warning (disable:4311)

// 定义获取计算机名称的接口函数
static bool GetComputerNameInterface(std::wstring& computerName)
{
	wchar_t name[MAX_COMPUTERNAME_LENGTH + 1];  // 宽字符缓冲区,计算机名最大长度
	DWORD size = sizeof(name) / sizeof(name[0]);

	if (GetComputerName(name, &size))
	{
		computerName = name;  // 将结果存储到传入的引用中
		return true;          // 成功返回 true
	}
	else
	{
		return false;         // 失败返回 false
	}
}

//static std::string	LocalUnicodeToUtf8(const std::wstring& strUnicode)
//{
//	string ret = "";
//	size_t len = strUnicode.size();
//	for (size_t i = 0; i < len; ++i)
//	{
//		wchar_t c = strUnicode[i];
//		if (c < 0x80)
//		{
//			ret.push_back(c & 0x7f);
//		}
//		else if (c < 0x0800)
//		{
//			ret.push_back((c >> 6) & 0x1f | 0xc0);
//			ret.push_back(c & 0x3f | 0x80);
//		}
//		else if (c < 0x010000)
//		{
//			ret.push_back((c >> 12) & 0x0f | 0xE0);
//			ret.push_back((c >> 6) & 0x3f | 0x80);
//			ret.push_back(c & 0x3f | 0x80);
//		}
//		else
//		{
//#if !defined(PLATFORM_WINDOWS)
//			ret.push_back((c >> 18) & 0x7 | 0xF0);
//			ret.push_back((c >> 12) & 0x3f | 0x80);
//			ret.push_back((c >> 6) & 0x3f | 0x80);
//			ret.push_back(c & 0x3f | 0x80);
//#endif
//		}
//	}
//#if defined(PLATFORM_WINDOWS)
//	{
//		int len = ::WideCharToMultiByte(CP_UTF8, 0, strUnicode.c_str(), -1, NULL, 0, NULL, NULL);
//		if (len == 0) return "";
//
//		char* putf8 = new char[len];
//		::WideCharToMultiByte(CP_UTF8, 0, strUnicode.c_str(), -1, putf8, len, NULL, NULL);
//
//		string strUtf8 = putf8;
//		delete[] putf8;
//		return strUtf8;
//		EASSERT(ret == strUtf8, "");
//	}
//#endif
//	return ret;
//}

static std::string LocalUnicodeToUtf8(const std::wstring& strUnicode)
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

string	UnicodeToANSI(wstring strUnicode)
{
	int len = ::WideCharToMultiByte(936, 0, strUnicode.c_str(), -1, NULL, 0, NULL, NULL);
	if (len == 0) return "";

	char* putf8 = new char[len];
	::WideCharToMultiByte(936, 0, strUnicode.c_str(), -1, putf8, len, NULL, NULL);

	string strAnsi = putf8;
	delete[] putf8;
	return strAnsi;
}

wstring	ANSIToUnicode(string strAnsi)
{
	int len = ::MultiByteToWideChar(936, 0, strAnsi.c_str(), -1, NULL, 0);
	if (len == 0) return L"";

	wchar_t* punicode = new wchar_t[len];
	::MultiByteToWideChar(936, 0, strAnsi.c_str(), -1, punicode, len);

	wstring wstrUnicode = punicode;
	delete[] punicode;
	return wstrUnicode;
}

static string GetClientSign2()
{
	char szMAC[128] = { "\0" };
	int nMacLen = sizeof(szMAC);
	char szDiskSN[128] = { "\0" };
	int nDiskSNLen = sizeof(szDiskSN);
	char szCpuInfo[128] = { "\0" };
	int nCpuInfoLen = sizeof(szCpuInfo);

	// Get all information and build a info string szSignature
	char szSignature[1024] = { 0 };
	char szSignature_md5[1024] = { 0 };
	char szMac_md5[200] = { 0 };
	char szCpuInfo_md5[200] = { 0 };
	char szDiskSN_md5[200] = { 0 };
	int nSignatureLen = 0;

	// Info string: MAC	+ DISK SN + VOLUME SN + USER NAME
	// MAC
	// First using Netbios to get the MAC Address
	if (CComputerInfo::GetMacAddress(szMAC, nMacLen, true))
	{
		md5_string((const unsigned char*)szMAC, szMac_md5, nMacLen);
		memcpy(szSignature_md5, szMac_md5, strlen(szMac_md5));
		strcat(szSignature_md5, ":");
		//nSignatureLen += nMacLen;
	}
	int nCpuInfoSize = 128, nNumProcessor = 0;
	if (CComputerInfo::GetCpuInfo(szCpuInfo, nCpuInfoSize, nNumProcessor))
	{
		md5_string((const unsigned char*)szCpuInfo, szCpuInfo_md5, nCpuInfoSize);
		memcpy(szSignature_md5 + strlen(szSignature_md5), szCpuInfo_md5, strlen(szCpuInfo_md5));
		strcat(szSignature_md5, ":");
		//nSignatureLen += nCpuInfoSize;
	}
	// Disk drive SN
	if (CComputerInfo::GetDiskSN(szDiskSN, nDiskSNLen))
	{
		md5_string((const unsigned char*)szDiskSN, szDiskSN_md5, nDiskSNLen - 1);
		memcpy(szSignature_md5 + strlen(szSignature_md5), szDiskSN_md5, strlen(szDiskSN_md5));
		//nSignatureLen += nDiskSNLen-1;
	}

	// 	szSignature[nSignatureLen] = '\0';
	// 
	wstring szsign = ANSIToUnicode(szSignature_md5);
	OutputDebugString(szsign.c_str());
	// 	md5_string((const unsigned char *)szSignature, szSignature_md5, nSignatureLen);
	return szSignature_md5;
}

static std::string ConstructJson(std::string traceId)
{
	// 创建 JSON 对象
	json_object* root = json_object_new_object();

	// 添加字符串字段
	json_object_object_add(root, "adTraceId", json_object_new_string(traceId.c_str()));

	// 转换为字符串
	const char* jsonString = json_object_to_json_string(root);

	// 手动移除空格（VS2008 兼容方式）
	// 手动移除空格（VS2008 兼容方式）
	std::string result(jsonString);
	std::string compactResult;
	bool insideString = false;

	// 遍历字符串，保留字符串值内的空格，删除结构空格
	for (size_t i = 0; i < result.size(); ++i)
	{
		char ch = result[i];

		// 遇到引号，切换 insideString 标志
		if (ch == '\"')
		{
			insideString = !insideString;
		}

		// 删除不在字符串内的空格
		if (!insideString && (ch == ' ' || ch == '\n' || ch == '\t'))
		{
			continue;  // 跳过空格
		}

		compactResult += ch;
	}


	//// 返回字符串并释放对象
	//std::string result(jsonString);
	json_object_put(root);  // 释放 JSON 对象内存

	return compactResult;
}

static std::string ConstructInitAdvJson(std::string m_deviceId)
{
	// 创建 JSON 对象
	json_object* root = json_object_new_object();

	// 添加字符串字段
	json_object_object_add(root, "deviceId", json_object_new_string(m_deviceId.c_str()));

	// 转换为字符串
	const char* jsonString = json_object_to_json_string(root);

	// 手动移除空格（VS2008 兼容方式）
	std::string result(jsonString);
	std::string compactResult;
	bool insideString = false;

	// 遍历字符串，保留字符串值内的空格，删除结构空格
	for (size_t i = 0; i < result.size(); ++i)
	{
		char ch = result[i];

		// 遇到引号，切换 insideString 标志
		if (ch == '\"')
		{
			insideString = !insideString;
		}

		// 删除不在字符串内的空格
		if (!insideString && (ch == ' ' || ch == '\n' || ch == '\t'))
		{
			continue;  // 跳过空格
		}

		compactResult += ch;
	}

	// 返回字符串并释放对象
	/*std::string result(jsonString);*/
	json_object_put(root);  // 释放 JSON 对象内存

	return compactResult;
}

static std::string	ConvertStringFromInt(int in_int)
{
	static const size_t digit_size = 100;
	char instr[digit_size + 1] = { 0 };
	sprintf(instr, "%d", in_int);
	return instr;
}

HttpRequest* RequestProcess::GetGetDynamicKeyRequest(const string& key)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetDynamicKey;
	request->url = "authen/getGuid.json?generateDynamicKey=1";

	if (key.length() > 0)
	{
		request->url2 = string("authen/getGuid.json?generateDynamicKey=1&key=") + UrlEncoder::encode_go_down(key.c_str());
	}

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetClientVKeyRequest(const char* tgt)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetClientVKey;
	string ticket = m_tgt; if (NULL != tgt) ticket = tgt;
	request->url = string("authen/getClientVKey.json?tgt=") + ticket;
	request->url2 = string("authen/getClientVKey.json?tgt=") + ticket;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::SendUserAccountRequest(const char* inputUserId)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SendUserAccount;
	request->url = string("authen/sendUserAccount.json?inputUserId=") + UrlEncoder::encode_go_down(GbkToUtf8(inputUserId).c_str());

	request->url2 = string("authen/sendUserAccount.json?inputUserId=") + UrlEncoder::encode_go_down(GbkToUtf8(inputUserId).c_str());


	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

static string GetMacAddress()
{
	static const size_t buffer_size = 0x100;
	char szMacAddress[buffer_size] = { 0 };

	try
	{
		PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
		ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
		// Make an initial call to GetAdaptersInfo to get the necessary size into the ulOutBufLen variable
		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
		{
			free(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
		}

		DWORD dwRetVal = 0;
		if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
		{
			for (PIP_ADAPTER_INFO pAdapter = pAdapterInfo; NULL != pAdapter; pAdapter = pAdapter->Next)
			{
				if (pAdapter->IpAddressList.IpAddress.String == "") continue;
				sprintf_s(szMacAddress, buffer_size, "%02X-%02X-%02X-%02X-%02X-%02X", pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2], pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);
				break;
			}
		}
		else
		{
			free(pAdapterInfo);
			return false;
		}

		free(pAdapterInfo);
		return szMacAddress;
	}
	catch (...)
	{
		return "";
	}
}

static string GetClientSign()
{
	char szMAC[128] = { "\0" };
	int nMacLen = sizeof(szMAC);
	char szDiskSN[128] = { "\0" };
	int nDiskSNLen = sizeof(szDiskSN);
	char szVolumeSN[128] = { "\0" };
	int nVolumeSNLen = sizeof(szVolumeSN);

	// Get all information and build a info string szSignature
	char szSignature[1024] = { 0 };
	char szSignature_md5[1024] = { 0 };
	int nSignatureLen = 0;

	// Info string: MAC	+ DISK SN + VOLUME SN + USER NAME
	// MAC
	// First using Netbios to get the MAC Address
	if (CComputerInfo::GetMacAddress(szMAC, nMacLen, true))
	{
		memcpy(szSignature, szMAC, nMacLen);
		nSignatureLen += nMacLen;
	}
	// Disk drive SN
	if (CComputerInfo::GetDiskSN(szDiskSN, nDiskSNLen))
	{
		memcpy(szSignature + nSignatureLen, szDiskSN, nDiskSNLen - 1);
		nSignatureLen += nDiskSNLen - 1;
	}

	// Computer Name
	char szHostName[128] = { "\0" };
	int nHostNameLen = sizeof(szHostName);
	if (CComputerInfo::GetComputerNameSD(szHostName, nHostNameLen))
	{
#ifdef _UNICODE
		string s = UnicodeToANSI((const wchar_t*)(szHostName));
#else
		string s = szHostName;
#endif
		memcpy(szSignature + nSignatureLen, s.c_str(), s.size());
		nSignatureLen += s.size();

	}

	szSignature[nSignatureLen] = '\0';

	md5_string((const unsigned char*)szSignature, szSignature_md5, nSignatureLen);
	return szSignature_md5;
}


HttpRequest* RequestProcess::GetStaticLoginRequest(const char* inputUserId, const char* password,
	int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int keepLoginFlag)
{
	return GetStaticLoginWithGameAccountRequest(inputUserId, password, accountDomain, autoLoginFlag, autoLoginKeepTime, isSupportGeetest, 0, keepLoginFlag);
}

HttpRequest* RequestProcess::GetStaticLoginRequest2(const char* inputUserId, const char* password,
	int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int keepLoginFlag)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_StaticLogin;
	request->url = string("authen/staticLogin.json?encryptFlag=0&inputUserId=") + UrlEncoder::encode_go_down(GbkToUtf8(inputUserId).c_str()) +
		"&password=" + UrlEncoder::encode_go_down(password) +
		"&guid=" + m_guid;

	request->url2 = string("authen/staticLogin.json?encryptFlag=1&inputUserId=") + UrlEncoder::encode_go_down(GbkToUtf8(inputUserId).c_str()) +
		"&password=" + UrlEncoder::encode_go_down(password) +
		"&guid=" + m_guid;

	char buff[1024];
	sprintf(buff, "&accountDomain=%d&autoLoginFlag=%d&autoLoginKeepTime=%d&isSupportGeetest=%d",
		accountDomain, autoLoginFlag, autoLoginKeepTime, isSupportGeetest);
	request->url += buff;

	//新增静密登录中的自动登录参数keepLoginFlag 20241009
	char buff2[1024];
	sprintf(buff2, "&keepLoginFlag=%d", keepLoginFlag);
	request->url += buff2;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetStaticLoginWithGameAccountRequest(const char* inputUserId, const char* password,
	int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int inputUserType, int keepLoginFlag, const char* scene)
{
	string strMacAddress = GetClientSign();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_StaticLogin;
	string InUserType;
	if (inputUserType == 0) {
		InUserType = "0";
	}
	else {
		InUserType = "1";
	}
	request->url = string("authen/staticLogin.json?checkCodeFlag=1&encryptFlag=1&inputUserId=") + UrlEncoder::encode_go_down(GbkToUtf8(inputUserId).c_str()) +
		"&password=" + UrlEncoder::encode_go_down(password) + "&mac=" + UrlEncoder::encode_go_down(strMacAddress.c_str()) +
		"&guid=" + m_guid + "&inputUserType=" + InUserType;

	char buff[1024];
	sprintf(buff, "&accountDomain=%d&autoLoginFlag=%d&autoLoginKeepTime=%d&supportPic=%d",
		accountDomain, autoLoginFlag, autoLoginKeepTime, isSupportGeetest);
	request->url += buff;
	//新增scene参数  20210611
	if (scene != NULL) {
		request->url += string("&scene=") + scene;
	}

	//新增静密登录中的自动登录参数keepLoginFlag 20241009
	char buff2[1024];
	sprintf(buff2, "&keepLoginFlag=%d", keepLoginFlag);
	request->url += buff2;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetCheckCodeLoginRequest(const char* checkCode, const char* challenge, const char* validate, const char* seccode, const char* outinfo, int keepLoginFlag, const char* captchaInfo)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CheckCodeLogin;

	request->url = string("authen/checkCodeLogin.json?guid=") + m_guid;
	if (NULL != checkCode) request->url += string("&password=") + checkCode;
	if (NULL != challenge) request->url += string("&challenge=") + challenge;
	if (NULL != validate) request->url += string("&validate=") + validate;
	if (NULL != seccode) request->url += string("&seccode=") + seccode;
	if (NULL != outinfo) request->url += string("&outInfo=") + outinfo;

	if (captchaInfo != NULL) {
		request->url += string("&captchaInfo=") + captchaInfo;
	}

	//新增静密登录中的自动登录参数keepLoginFlag 20241009
	char buff[1024];
	sprintf(buff, "&keepLoginFlag=%d", keepLoginFlag);
	request->url += buff;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetSendPushMessageVerifyCheckCodeRequest(const char* pushMsgSessionKey, const char* checkCode, int keepLoginFlag, const char* captchaInfo)
{
	HttpRequest* request = new HttpRequest;

	request->requestCode = REQ_SendPushMessageVerifyCheckCode;

	request->url = string("authen/sendPushMessageVerifyCheckCode.json?pushMsgSessionKey=") + pushMsgSessionKey +
		"&password=" + checkCode;

	if (captchaInfo != NULL) {
		request->url += string("&captchaInfo=") + UrlEncoder::encode_go_down(captchaInfo);
	}

	//新增一键登录图验中的自动登录参数keepLoginFlag 20241009
	char buff[1024];
	sprintf(buff, "&keepLoginFlag=%d", keepLoginFlag);
	request->url += buff;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);

	return request;
}

HttpRequest* RequestProcess::GetDynamicLoginRequest(const char* password, int keepLoginFlag)
{
	HttpRequest* request = new HttpRequest;
	if (m_loginType == "3") // voice use post method
	{
		request->requestCode = REQ_DynamicLoginVoice;
		request->url = "authen/dynamicLogin.json";
		request->postData = string("encryptFlag=0&guid=") + m_guid +
			"&loginType=" + m_loginType +
			"&password=" + UrlEncoder::encode_go_down(password);

		//新增安全卡动密登录中的自动登录参数keepLoginFlag  20241009
		char buff[1024];
		sprintf(buff, "&keepLoginFlag=%d", keepLoginFlag);
		request->url += buff;

		SetParamToUrl(request->url, &m_mapReqParams);

		SetCommonParam(request, false);
	}
	else if (m_loginType == "2")
	{
		request->requestCode = REQ_DynamicLogin;
		request->url = string("authen/dynamicLogin.json?encryptFlag=1&guid=") + m_guid +
			"&loginType=" + m_loginType +
			"&password=" + UrlEncoder::encode_go_down(password); // 安全卡需要encode

		//新增安全卡动密登录中的自动登录参数keepLoginFlag  20241009
		char buff[1024];
		sprintf(buff, "&keepLoginFlag=%d", keepLoginFlag);
		request->url += buff;

		SetParamToUrl(request->url, &m_mapReqParams);

		SetCommonParam(request);
	}
	else
	{
		request->requestCode = REQ_DynamicLogin;
		request->url = string("authen/dynamicLogin.json?encryptFlag=0&guid=") + m_guid +
			"&loginType=" + m_loginType +
			"&password=" + UrlEncoder::encode_go_down(password); // 安全卡需要encode

		//新增安全卡动密登录中的自动登录参数keepLoginFlag  20241009
		char buff[1024];
		sprintf(buff, "&keepLoginFlag=%d", keepLoginFlag);
		request->url += buff;

		SetParamToUrl(request->url, &m_mapReqParams);

		SetCommonParam(request);
	}

	return request;
}

HttpRequest* RequestProcess::GetFcmLoginRequest(const char* realName, const char* idCard, const char* email, int keepLoginFlag)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_FcmLogin;
	string strUeAppId = "PC-KYC#" + IntToStr(m_appid);

	request->url = string("authen/fcmLogin.json?realName=") +
		UrlEncoder::encode_go_down(GbkToUtf8(realName).c_str()) +
		"&idCard=" + UrlEncoder::encode_go_down(idCard) +
		"&email=" + UrlEncoder::encode_go_down(email) +
		"&ueAppId=" + UrlEncoder::encode_go_down(strUeAppId.c_str()) + "&ueFlowId=" + m_ueFlowId +
		"&tgt=" + m_tgt;

	//新增实名登录中的自动登录参数keepLoginFlag 20241009
	char buff[1024];
	sprintf(buff, "&keepLoginFlag=%d", keepLoginFlag);
	request->url += buff;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetAutoLoginRequest(const char* autoLoginSessionKey)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_AutoLogin;
	request->url = string("authen/autoLogin.json?autoLoginSessionKey=") + autoLoginSessionKey + "&guid=" + m_guid;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetSsoLoginRequest(const char* tgt, const char* scene)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SsoLogin;
	string ticket = m_tgt; if (NULL != tgt) ticket = tgt;
	if (scene == NULL)
	{
		request->url = string("authen/ssoLogin.json?tgt=") + ticket + "&guid=" + m_guid + "&scene=" + "";
	}
	else
	{
		request->url = string("authen/ssoLogin.json?tgt=") + ticket + "&guid=" + m_guid + "&scene=" + scene;
	}


	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

//盒子登录游戏返回授权码
HttpRequest* RequestProcess::GetAuthCodeRequest(const char* tgt, const char* scene)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SsoLoginAuthCode;
	string ticket = m_tgt; if (NULL != tgt) ticket = tgt;
	if (scene == NULL)
	{
		request->url = string("authen/getSsoAuthorization?tgt=") + ticket + "&guid=" + m_guid;
	}
	else
	{
		request->url = string("authen/getSsoAuthorization?tgt=") + ticket + "&guid=" + m_guid;
	}


	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request, scene);
	return request;
}

//授权码验证
HttpRequest* RequestProcess::GetCheckAuthCodeRequest(const char* authCode)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CheckAuthCode;
	request->url = string("authen/ssoAuthorizationLogin?authorization=") + authCode;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request, "V3Launcher");
	return request;
}

HttpRequest* RequestProcess::GetSsoLoginRequest(const char* tgt, const char* scene, int appId)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SsoLogin;
	string ticket = m_tgt; if (NULL != tgt) ticket = tgt;
	if (scene == NULL)
	{
		request->url = string("authen/ssoLogin.json?tgt=") + ticket + "&guid=" + m_guid + "&scene=" + "";
	}
	else
	{
		request->url = string("authen/ssoLogin.json?tgt=") + ticket + "&guid=" + m_guid + "&scene=" + scene;
	}

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request, appId);
	return request;
}

HttpRequest* RequestProcess::GetTicketRequest()
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetTicket;
	request->url = string("authen/lsc/getTicket?tgt=") + m_tgt + "&guid=" + m_guid;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetFastLoginRequest()
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_FastLogin;
	request->url = string("authen/fastLogin.json?tgt=") + m_tgt + "&guid=" + m_guid;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetPhoneCheckCodeLoginRequest(const char* checkCode)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_PhoneCheckCodeLogin;
	request->url = string("authen/phoneCheckCodeLogin.json?checkCode=") + checkCode +
		"&checkCodeSessionKey=" + m_checkCodeSessionKey + "&guid=" + m_guid;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetLogoutRequest()
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_Logout;
	request->url = string("authen/logout.json?tgt=") + m_tgt;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	m_tgt = "";
	return request;
}

HttpRequest* RequestProcess::GetQrCodeLoginRequest(int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CodeKeyLogin;
	request->url = string("authen/codeKeyLogin.json?codeKey=") + m_codeKey + "&guid=" + m_guid + "&autoLoginFlag=" + IntToStr(autoLoginFlag) + "&autoLoginKeepTime=" + IntToStr(autoLoginKeepTime);

	//新增扫码登录中的自动登录参数  20241009
	char buff[1024];
	sprintf(buff, "&keepLoginFlag=%d", keepLoginFlag);
	request->url += buff;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetCheckAccoutTypeRequest(const char* inputUserId)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CheckAccountType;
	request->url = string("authen/checkAccountType.json?inputUserId=") +
		UrlEncoder::encode_go_down(GbkToUtf8(inputUserId).c_str())
		+ "&guid=" + m_guid;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetSendPhoneCheckCodeRequest(const char* inputUserId, int type)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SendPhoneCheckCode;
	request->url = string("authen/sendPhoneCheckCode.json?inputUserId=") +
		UrlEncoder::encode_go_down(GbkToUtf8(inputUserId).c_str());
	char buff[1024];
	sprintf(buff, "&type=%d", type);
	request->url += buff;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetSessionIdStatesRequest(const char* sessioId)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetSessionIdStates;

	request->url = string("authen/getloginstates.json?");

	char szBuffer[4096] = { 0 };
	strncpy_s(szBuffer, sessioId, sizeof(szBuffer) - 1);

	string data = "";
	char* next_token = NULL;
	char* token = strtok_s(szBuffer, " ", &next_token);
	while (token != NULL)
	{
		if (data.size() != 0) data += "&";
		data += "tgtArray="; data += token;
		token = strtok_s(NULL, " ", &next_token);
	}
	request->url += data;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGetQrCodeRequest()
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetQrCode;
	request->url = string("authen/getCodeKey.json?");
	if (!m_codeKey.empty())
		request->url += "&codeKey=" + m_codeKey;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGetLoginStateRequest()
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetLoginStatus;
	request->url = string("authen/getLoginState.json?tgt=") + m_tgt;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGetLoginStateRequest2(const char* tgt)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetLoginStatus;
	request->url = string("authen/getLoginState.json?tgt=") + tgt;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetExtendLoginStateRequest(const char* tgt)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_ExtendLoginState;
	string ticket = m_tgt; if (NULL != tgt) ticket = tgt;
	request->url = string("authen/extendLoginState.json?tgt=") + ticket;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	//set_signture(request);
	return request;
}

HttpRequest* RequestProcess::GetPushMessageStatusRequest(const char* inputUserId)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetPushMessageStatus;
	request->url = string("authen/getPushMessageStatus.json?inputUserId=") + UrlEncoder::encode_go_down(GbkToUtf8(inputUserId).c_str());

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetPushMessageLoginRequest(int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_PushMessageLogin;
	request->url = "authen/pushMessageLogin.json?pushMsgSessionKey=" + m_pushMsgSessionKey + "&guid=" + m_guid + "&autoLoginFlag=" + IntToStr(autoLoginFlag) + "&autoLoginKeepTime=" + IntToStr(autoLoginKeepTime);

	//新增一键登录中的自动登录参数keepLoginFlag  20241009
	char buff[1024];
	sprintf(buff, "&keepLoginFlag=%d", keepLoginFlag);
	request->url += buff;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetCancelPushMessageLoginRequest()
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CancelPushMessageLogin;
	request->url = "authen/cancelPushMessageLogin.json?pushMsgSessionKey=" + m_pushMsgSessionKey + "&guid=" + m_guid;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetSendPushMessageRequest(const char* inputUserId, const char* scene)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SendPushMessage;
	request->url = string("authen/sendPushMessage.json?inputUserId=") + UrlEncoder::encode_go_down(GbkToUtf8(inputUserId).c_str());
	if (scene != NULL)
	{
		request->url += string("&scene=") + scene;
	}

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetRltLoginRequest(const char* vkey)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_RltLogin;
	request->url = string("authen/rltLogin.json?vkey=") + vkey + "&guid=" + m_guid;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetRltLoginKeepLoginRequest(const char* vkey, int keepLoginFlag)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_RltLoginKeepLogin;
	request->url = string("authen/rltLogin.json?vkey=") + vkey + "&guid=" + m_guid;

	//新增一键登录中的自动登录参数keepLoginFlag  20250909
	char buff[1024];
	sprintf(buff, "&keepLoginFlag=%d", keepLoginFlag);
	request->url += buff;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGetAccountInfoRequest(const char* tgt)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetAccountInfo;
	request->url = string("authen/getAccountInfo.json?tgt=") + tgt;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGetLoginHistoryRequest(const char* tgt, int queryNumber)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetLoginHistory;
	string ticket = m_tgt; if (NULL != tgt) ticket = tgt;
	request->url = string("authen/getLoginHistory.json?queryNumber=") + IntToStr(queryNumber) + "&tgt=" + ticket;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGetLoginUserInfoRequest(const char* tgt)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetLoginUserInfo;
	request->url = string("authen/getLoginUserInfo.json?") + "tgt=" + tgt;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetSetLoginUserInfoRequest(const char* tgt, const char* notename)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SetLoginUserInfo;
	request->url = string("authen/setLoginUserInfo.json?") + "tgt=" + tgt + "&notename=" + UrlEncoder::encode_go_down(GbkToUtf8(notename).c_str());

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGetLoginAreaInfoRequest(int nAppId)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetLoginAreaInfo;
	request->url = string("authen/getLoginAreaInfo.json?") + "appCode=" + IntToStr(nAppId);

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetPublicKeyRequest()
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetPublicKey;
	request->url2 = string("authen/getPublicKey.json?");

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

// HttpRequest* RequestProcess::GetGuidRequest(const string& key)
// {
// 	HttpRequest* request = new HttpRequest;
// 	request->requestCode = REQ_GetGuid;
// 	request->url = string("authen/getGuid.json?generateDynamicKey=1&key=") + key;
// 	SetCommonParam(request);
// 	return request;
// }

HttpRequest* RequestProcess::GetPromotionInfoRequest()
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetPromotionInfo;
	request->url = string("authen/getPromotionInfo.json?tgt=") + m_tgt;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetPromotionInfoConfirmRequest(int days)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_PromotionInfoConfirm;
	request->url = string("authen/promotionInfoConfirm.json?promotionValue=") + IntToStr(days) + "&tgt=" + m_tgt + "&promotionId=" + m_promotionId;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetAccountGroupListRequest(const char* tgt)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetAccountGroup;
	request->url = string("authen/getAccountGroup?serviceUrl=") + UrlEncoder::encode_go_down("http://www.sdo.com") + "&tgt=" + tgt;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetAccountGroupLoginRequest(const char* tgt, const char* sndaId, int autoLoginFlag, int autoLoginKeepTime)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_AccountGroupLogin;

	request->url = string("authen/accountGroupLogin?serviceUrl=") + UrlEncoder::encode_go_down("http://www.sdo.com") + "&tgt=" + tgt + "&sndaId=" + sndaId + "&autoLoginFlag=" + IntToStr(autoLoginFlag) + "&autoLoginKeepTime=" + IntToStr(autoLoginKeepTime);

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetThirdPartyPollingLoginRequest(const char* companyId, int autoLoginFlag, int autoLoginKeepTime)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_ThirdPartyPollingLogin;

	request->url = string("authen/thirdPartyPollingLogin?guid=") + m_guid + "&CompanyId" + companyId + "&autoLoginFlag=" + IntToStr(autoLoginFlag) + "&autoLoginKeepTime=" + IntToStr(autoLoginKeepTime);

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetThirdPartyLoginRequest(const char* companyId, const char* token, int autoLoginFlag, int autoLoginKeepTime, const char* scene, const char* phone, const char* smsCode)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_ThirdPartyLogin;

	request->url = string("authen/thirdPartyLogin?guid=") + m_guid + "&CompanyId=" + companyId + "&token=" + token + "&autoLoginFlag=" + IntToStr(autoLoginFlag) + "&autoLoginKeepTime=" + IntToStr(autoLoginKeepTime);

	if (scene != NULL)
	{
		request->url += string("&scene=") + scene;
	}

	if (phone != NULL)
	{
		request->url += string("&phone=") + phone;
	}

	if (smsCode != NULL)
	{
		request->url += string("&smsCode=") + smsCode;
	}

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetCloudGameLoginRequest(const char* tgt, const char* scene/*=NULL*/)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CloudGameLogin;

	request->url = string("authen/cloudGameLogin?guid=") + m_guid + "&tgt=" + tgt + string("&scene=") + scene;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetSendMiGuSmsRequest(const char* phone)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SendMiGuSms;

	request->url = string("authen/lsc/MiGuSMSSend?guid=") + m_guid + "&mobileNum=" + phone;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetSendSmsRequest(const char* smsSessionKey, const char* phone, int smsType, const char* scene)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SendSms;

	request->url = string("authen/sendSms.json?guid=") + m_guid + "&inputUserId=" + phone + "&smsType=" + IntToStr(smsType);
	if (smsSessionKey != NULL) request->url += string("&smsSessionKey=") + smsSessionKey;

	//新增scene参数  20210611
	if (scene != NULL) {
		request->url += string("&scene=") + scene;
	}

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetCheckCodeToSendSmsRequest(const char* smsSessionKey, const char* checkCode, const char* captchaInfo)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CheckCodeToSendSms;

	request->url = string("authen/sendsmsVerifyCheckCode.json?guid=") + m_guid + "&smsSessionKey=" + smsSessionKey;
	if (checkCode != NULL) {
		request->url += string("&checkCode=") + checkCode;
	}
	if (captchaInfo != NULL) {
		request->url += string("&captchaInfo=") + UrlEncoder::encode_go_down(captchaInfo);
	}

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetSmsLoginRequest(const char* smsSessionKey, const char* smsCode, int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SmsLogin;

	request->url = string("authen/smsLogin.json?guid=") + m_guid + "&smsSessionKey=" + smsSessionKey + "&smsCode=" + smsCode + "&autoLoginFlag=" + IntToStr(autoLoginFlag) + "&autoLoginKeepTime=" + IntToStr(autoLoginKeepTime);

	//新增一键注册登录中的自动登录参数keepLoginFlag  20241009
	char buff[1024];
	sprintf(buff, "&keepLoginFlag=%d", keepLoginFlag);
	request->url += buff;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetUserPrivacyConfig(const char* scene, int privacypolicyversion, int serviceAgreementVersion)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_UserPrivacyConfig;
	request->hostName = "utility.sdoprofile.com";
	request->port = m_port;
	request->hostName2 = "utility.sdoprofile.com";
	request->port2 = m_port2;
	request->timeout = 3000;
	request->timeout2 = 3000;
	request->method = "GET";

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	request->url = string("agreement/user?") + "appid=" + IntToStr(m_appid) + "&scene=" + scene
		+ "&privacypolicyversion=" + IntToStr(privacypolicyversion)
		+ "&serviceAgreementVersion=" + IntToStr(serviceAgreementVersion);
	return request;
}

HttpRequest* RequestProcess::GetNewVersionUserPrivacyConfig(const char* scene, int privacypolicyversion)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_NewVersionUserPrivacyConfig;
	request->hostName = "utility.sdoprofile.com";
	request->port = m_port;
	request->hostName2 = "utility.sdoprofile.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	request->url = string("agreement/all?") + "appid=" + IntToStr(m_appid) + "&channelCode=" + "windows" + "&platform=" + "windows" + "&scene=" + scene;

	return request;
}

HttpRequest* RequestProcess::GetFaceVerifyInit(const char* scene)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_FaceVerifyInit;
	request->hostName = "gfc.sdo.com";
	request->port = m_port;
	request->hostName2 = "gfc.sdo.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	request->url = string("api/faceVerify/init?authenType=1&appId=") + IntToStr(m_appid) + "&scene=" + scene + "&deviceId="
		+ m_deviceId + "&authenToken=" + m_tgt + "&areaId=" + IntToStr(m_areaid) + "&bizVersion=" + m_productVersion;
	return request;
}

HttpRequest* RequestProcess::CreateWeGameOrder(const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
	const char* szAreaId, const char* szExtend, const char* szSign, const char* channel)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CreateWeGameOrder;
	request->hostName = "mgame.sdo.com";
	request->port = m_port;
	request->hostName2 = "mgame.sdo.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	//request->url = string("v1/gchannel/third/pay/wegame/pc/order?authenType=1&appId=") + IntToStr(m_appid) + "&ticket=" + ticket + "&areaId=" + szAreaId
	//	+ "&groupId=" + szGroupId + "&productId=" + szProductId + "&gameOrderNo=" + szOrderId + "&ext=" + szExtend + "&sign=" + szSign + "&deviceId="
	//	+ m_deviceId + "&authenToken=" + m_tgt + "&channel=" + channel;
	request->url = string("/v1/gchannel/third/pay/uwo/pc/order?authenType=1&appId=") + IntToStr(m_appid) + "&ticket=" + ticket + "&areaId=" + szAreaId
		+ "&groupId=" + szGroupId + "&productId=" + szProductId + "&gameOrderNo=" + szOrderId + "&ext=" + szExtend + "&sign=" + szSign + "&deviceId="
		+ m_deviceId + "&authenToken=" + m_tgt + "&channel=" + channel;

	if (!m_productVersion.empty())
	{
		request->url += "&version=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}

	char szMAC[128] = { "\0" };
	int nMacLen = sizeof(szMAC);
	CComputerInfo::GetMacAddress(szMAC, nMacLen, true);
	request->url += "&mac=" + string(szMAC);

	return request;
}

HttpRequest* RequestProcess::GetThirdLoginSkin()
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_ThirdLoginSkin;
	request->url = string("authen/getLoginState.json?tgt=") + m_tgt;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::CreateCQ4Order(const char* ticket, const char* szOrderId,
	const char* szProductId, const char* szGroupId, const char* szAreaId, const char* szExtend,
	const char* szSign, const char* channel, int iSSandboxAccount)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CreateCQ4Order;
	if (iSSandboxAccount == 0)
	{
		request->hostName = "mgame.sdo.com";
		request->port = m_port;
		request->hostName2 = "mgame.sdo.com";
		request->port2 = m_port2;
	}
	else
	{
		request->hostName = "mgame-gray.sdo.com";
		request->port = m_port;
		request->hostName2 = "mgame-gray.sdo.com";
		request->port2 = m_port2;
	}

	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";

	std::string extendInfo;
	if (adTraceId.size() > 0)
	{
		extendInfo = ConstructJson(adTraceId);
	}
	std::string extendInfo_encode = UrlEncoder::encode_go_down(extendInfo.c_str());

	request->url = string("v1/pay/pc/order?authenType=1&appId=") + IntToStr(m_appid)
		+ "&ticket=" + ticket + "&areaId=" + szAreaId
		+ "&groupId=" + szGroupId + "&productId=" + szProductId + "&gameOrderNo="
		+ szOrderId + "&ext=" + szExtend + "&sign=" + szSign + "&deviceId="
		+ m_deviceId + "&authenToken=" + m_tgt + "&channel=" + channel
		+ "&extendInfo=" + extendInfo_encode + "&channelId=" + LoginClient::getChannelIdExtern();

	if (!m_productVersion.empty())
	{
		request->url += "&version=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}

	char szMAC[128] = { "\0" };
	int nMacLen = sizeof(szMAC);
	CComputerInfo::GetMacAddress(szMAC, nMacLen, true);
	request->url += "&mac=" + string(szMAC);

	return request;
}

HttpRequest* RequestProcess::CreateCQ4Query(const char* ticket, const char* szOrderId,
	const char* szSign, const char* channel, int iSSandboxAccount)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CreateCQ4Query;
	if (iSSandboxAccount == 0)
	{
		request->hostName = "mgame.sdo.com";
		request->port = m_port;
		request->hostName2 = "mgame.sdo.com";
		request->port2 = m_port2;
	}
	else
	{
		request->hostName = "mgame-gray.sdo.com";
		request->port = m_port;
		request->hostName2 = "mgame-gray.sdo.com";
		request->port2 = m_port2;
	}

	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	std::string extendInfo;
	if (adTraceId.size() > 0)
	{
		extendInfo = ConstructJson(adTraceId);
	}
	std::string extendInfo_encode = UrlEncoder::encode_go_down(extendInfo.c_str());
	request->url = string("v1/pay/pc/order/query?authenType=1&appId=")
		+ IntToStr(m_appid) + "&ticket=" + ticket + "&orderNo=" + szOrderId
		+ "&sign=" + szSign + "&authenToken=" + m_tgt + "&channel=" + channel
		+ "&extendInfo=" + extendInfo_encode + "&channelId=" + LoginClient::getChannelIdExtern();

	return request;
}

HttpRequest* RequestProcess::CreateSteamChannelOrder(const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
	const char* szAreaId, const char* szExtend, const char* szSign, const char* channel)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CreateSteamChannelOrder;
	request->hostName = "mgame.sdo.com";
	request->port = m_port;
	request->hostName2 = "mgame.sdo.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	request->url = string("v1/gchannel/third/pay/steam/pc/order?authenType=1&appId=") + IntToStr(m_appid) + "&ticket=" + ticket + "&areaId=" + szAreaId
		+ "&groupId=" + szGroupId + "&productId=" + szProductId + "&gameOrderNo=" + szOrderId + "&ext=" + szExtend + "&sign=" + szSign + "&deviceId="
		+ m_deviceId + "&authenToken=" + m_tgt + "&channel=" + channel;


	if (!m_productVersion.empty())
	{
		request->url += "&version=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}

	char szMAC[128] = { "\0" };
	int nMacLen = sizeof(szMAC);
	CComputerInfo::GetMacAddress(szMAC, nMacLen, true);
	request->url += "&mac=" + string(szMAC);

	return request;
}

HttpRequest* RequestProcess::CreateQQGameOrder(const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
	const char* szAreaId, const char* szExtend, const char* szOpenId, const char* szOpenKey, const char* szPfKey, const char* szSign, const char* channel)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CreateQQGameOrder;
	request->hostName = "mgame.sdo.com";
	request->port = m_port;
	request->hostName2 = "mgame.sdo.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	//request->url = string("v1/gchannel/third/pay/qq/pc/order?appId=")+IntToStr(m_appid)+"&ticket="+ticket+"&areaId="+szAreaId
	//	+"&groupId="+szGroupId+"&productId="+szProductId+"&gameOrderNo="+szOrderId+"&ext="+szExtend+"&sign="+ szSign+"&deviceId="
	//	+ m_deviceId+"&openid="+szOpenId+"&openkey="+szOpenKey+"&pfkey="+szPfKey+"&channel="+channel;
	request->url = string("v1/gchannel/third/pay/qq/pc/order?appId=") + IntToStr(m_appid) + "&ticket=" + ticket + "&areaId=" + szAreaId
		+ "&groupId=" + szGroupId + "&productId=" + szProductId + "&gameOrderNo=" + szOrderId + "&ext=" + szExtend + "&sign=" + szSign + "&deviceId="
		+ m_deviceId + "&openid=" + szOpenId + "&openkey=" + szOpenKey + "&pfkey=" + szPfKey + "&channel=" + channel + "&pf=qqgame&goodsurl=asdf";

	if (!m_productVersion.empty())
	{
		request->url += "&version=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}

	char szMAC[128] = { "\0" };
	int nMacLen = sizeof(szMAC);
	CComputerInfo::GetMacAddress(szMAC, nMacLen, true);
	request->url += "&mac=" + string(szMAC);

	return request;
}

HttpRequest* RequestProcess::WeGameStatus()
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_WeGameStatus;
	request->url = string("authen/lsc/wegameStatus?tgt=") + m_tgt + "&guid=" + m_guid;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::CreateLxOrder(const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
	const char* szAreaId, const char* szExtend, const char* szSign, const char* channel, const char* szLenovoTgt, const char* szRole)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CreateLxOrder;
	request->hostName = "mgame.sdo.com";
	request->port = m_port;
	request->hostName2 = "mgame.sdo.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	request->url = string("/v1/gchannel/third/pay/lenovo/pc/order?authenType=1&appId=") + IntToStr(m_appid) + "&ticket=" + ticket + "&areaId=" + szAreaId
		+ "&groupId=" + szGroupId + "&productId=" + szProductId + "&gameOrderNo=" + szOrderId + "&ext=" + szExtend + "&sign=" + szSign + "&deviceId="
		+ m_deviceId + "&authenToken=" + m_tgt + "&lenovoTgt=" + szLenovoTgt + "&channel=" + channel + "&role=" + szRole;


	if (!m_productVersion.empty())
	{
		request->url += "&version=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}

	char szMAC[128] = { "\0" };
	int nMacLen = sizeof(szMAC);
	CComputerInfo::GetMacAddress(szMAC, nMacLen, true);
	request->url += "&mac=" + string(szMAC);

	return request;
}

HttpRequest* RequestProcess::GetFaceCodeResult(const char* contextId)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_FaceCodeResult;
	request->hostName = "gfc.sdo.com";
	request->port = m_port;
	request->hostName2 = "gfc.sdo.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	request->url = string("api/faceVerify/getCodeResult?contextId=") + contextId + "&deviceId=" + m_deviceId + "&areaId=" + IntToStr(m_areaid) + "&bizVersion=" + m_productVersion;
	return request;
}

HttpRequest* RequestProcess::SendAction(const char* contextId, int action)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_FaceSendAction;
	request->hostName = "gfc.sdo.com";
	request->port = m_port;
	request->hostName2 = "gfc.sdo.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	request->url = string("api/faceVerify/sendAction?orderNo=") + contextId + "&deviceId=" + m_deviceId + "&areaId=" + IntToStr(m_areaid) + "&bizVersion=" + m_productVersion + "&action=" + IntToStr(action);
	return request;
}

HttpRequest* RequestProcess::UeInitClient(const char* hash) {
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_UeInitClient;
	string strUeAppId = "PC-KYC#" + IntToStr(m_appid);
	request->url = string("authen/ueInitClient?guid=") + m_guid + "&tgt=" + m_tgt + "&ueAppId=" + UrlEncoder::encode_go_down(strUeAppId.c_str()) + "&hash=" + hash + "&ueFlowId=" + m_ueFlowId;
	SetParamToUrl(request->url, &m_mapReqParams);
	SetCommonParam(request);
	return request; return request;
}

HttpRequest* RequestProcess::UeReport(const char* szExtendData) {
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_UeReport;
	string strUeAppId = "PC-KYC#" + IntToStr(m_appid);
	request->url = "authen/ueReport";
	request->postData = string("guid=") + m_guid +
		"&ueAppId=" + UrlEncoder::encode_go_down(strUeAppId.c_str()) +
		"&extendData=" + UrlEncoder::encode_go_down(szExtendData) +
		"&tgt=" + m_tgt;

	SetParamToUrl(request->url, &m_mapReqParams);
	SetCommonParam(request, false);

	return request;
}

HttpRequest* RequestProcess::SendSteamPayResult(const char* szSteamId, const char* szOrderId)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SteamPayResult;
	request->hostName = "paygate.sdo.com";
	request->port = m_port;
	request->hostName2 = "paygate.sdo.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";
	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}
	request->url = string("/notify/steam?") + "deviceId=" + m_deviceId + "&tgt=" + m_tgt + "&areaId=" + IntToStr(m_areaid) + "&steamId=" + szSteamId + "&orderId=" + szOrderId + "&appId=" + IntToStr(m_appid);
	if (!m_productVersion.empty())
	{
		request->url += "&version=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}

	char szMAC[128] = { "\0" };
	int nMacLen = sizeof(szMAC);
	CComputerInfo::GetMacAddress(szMAC, nMacLen, true);
	request->url += "&mac=" + string(szMAC);
	return request;
}

HttpRequest* RequestProcess::SendSteamChannelPayResult(CONST char* szChannel, const char* szTicket, const char* szOrderId)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_SteamChannelPayResult;
	request->hostName = "paygate.sdo.com";
	request->port = m_port;
	request->hostName2 = "paygate.sdo.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;
	request->method = "GET";
	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}
	request->url = string("/v1/gchannel/third/pay/steam/pc/notify?") + "deviceId=" + m_deviceId + "&channel=" + szChannel + "&ticket=" + szTicket + "&appId=" + IntToStr(m_appid) + "&areaId=" + IntToStr(m_areaid) + "&payOrderNo=" + szOrderId;
	if (!m_productVersion.empty())
	{
		request->url += "&version=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}

	char szMAC[128] = { "\0" };
	int nMacLen = sizeof(szMAC);
	CComputerInfo::GetMacAddress(szMAC, nMacLen, true);
	request->url += "&mac=" + string(szMAC);
	return request;
}

HttpRequest* RequestProcess::GetKickoffAccountVerifyRequest(const char* tgt)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_KickoffVerify;
	request->url = string("authen/kickoffAccountVerify.json?tgt=") + tgt;

	SetParamToUrl(request->url, &m_mapReqParams);
	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetKickoffProtectCodeRequest(const char* tgt, const char* protectCode)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_KickOffVerifyCheckCode;
	request->url = string("authen/kickoffAccountVerifyCheckCode.json?tgt=") + tgt + "&password=" + protectCode;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetKickoffAccountRequest(const char* tgt, const char* checkCode, int nKickoffAppid, int nKickoffAreaid)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_KickoffAccount;
	request->url = string("authen/kickoffAccount.json?tgt=") + tgt + "&password=" + checkCode + "&kickoffAppId=" + IntToStr(nKickoffAppid) + "&kickoffAreaId=" + IntToStr(nKickoffAreaid);

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}
//QQGame
HttpRequest* RequestProcess::GetQQGameLoginRequest(const char* openid, const char* openkey, bool is_limited, int company_id)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_QQGameLogin;
	string s_is_limited;
	if (is_limited) {
		s_is_limited = "1";
	}
	else {
		s_is_limited = "0";
	}
	char szCopmpanyId[20];
	sprintf(szCopmpanyId, "%d", company_id);
	request->url = string("authen/thirdPartyLogin?thridUserId=") + openid + "&islimited=" + s_is_limited + "&token=" + openkey + "&companyid=" + szCopmpanyId + "&autoLoginFlag=0&autoLoginKeepTime=0" + "&extend=" + openkey;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}
HttpRequest* RequestProcess::GetQQGameIsLoginRequest(const char* openid, const char* openkey, int company_id)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_QQGameIsLogin;
	char szCopmpanyId[20];
	sprintf(szCopmpanyId, "%d", company_id);
	request->url = string("authen/lsc/qqgameIsLogin?thridUserId=") + openid + "&token=" + openkey + "&companyid=" + szCopmpanyId;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}
//WeGame
HttpRequest* RequestProcess::GetWeGameLoginRequest(const char* rail_id, const char* rail_session_ticket, bool is_limited)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_WeGameLogin;
	string s_is_limited;
	if (is_limited) {
		s_is_limited = "1";
	}
	else {
		s_is_limited = "0";
	}
	request->url = string("authen/thirdPartyLogin?thridUserId=") + rail_id + "&islimited=" + s_is_limited + "&token=" + rail_session_ticket + "&companyid=310&autoLoginFlag=0&autoLoginKeepTime=0";

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}
HttpRequest* RequestProcess::GetWeGameLoginRequest(const char* rail_id, const char* rail_session_ticket, bool is_limited, int company_id)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_WeGameLogin;
	string s_is_limited;
	if (is_limited) {
		s_is_limited = "1";
	}
	else {
		s_is_limited = "0";
	}
	char szCopmpanyId[20];
	sprintf(szCopmpanyId, "%d", company_id);

	//WeGame传过来的票据Encode
	std::string urlencode_token = UrlEncoder::encode_go_down(rail_session_ticket);
	if (m_appid == 39)
	{
		request->url = string("authen/thirdPartyLogin?thridUserId=") + rail_id + "&islimited=" + s_is_limited + "&token=" + urlencode_token + "&companyid=" + szCopmpanyId + "&autoLoginFlag=0&autoLoginKeepTime=0";
	}
	else
	{
		request->url = string("authen/thirdPartyLogin?thridUserId=") + rail_id + "&islimited=" + s_is_limited + "&token=" + rail_session_ticket + "&companyid=" + szCopmpanyId + "&autoLoginFlag=0&autoLoginKeepTime=0";
	}

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}
//三方登陆
HttpRequest* RequestProcess::GetThirdLoginRequest(const char* third_token, const char* companyId, const char* scene,
	const char* phone, const char* smsCode, const char* extend, const char* szIsLimited)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_LenovoLogin;
	request->url = string("authen/thirdPartyLogin?token=") + third_token + "&companyid=" + companyId;
	if (scene != NULL)
	{
		request->url += string("&scene=") + scene;
	}

	if (phone != NULL)
	{
		request->url += string("&phone=") + phone;
	}

	if (smsCode != NULL)
	{
		request->url += string("&smsCode=") + smsCode;
	}

	if (extend != NULL)
	{
		request->url += string("&extend=") + extend;
	}

	if (szIsLimited != NULL) {
		request->url += string("&islimited=") + szIsLimited;
	}

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

//三方登陆，QQ、WX、WB
HttpRequest* RequestProcess::GetForThirdLoginRequest(const char* third_token, const char* companyId, const char* scene,
	const char* phone, const char* smsCode, const char* extend, const char* szIsLimited)
{
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_ThirdLogin;
	request->url = string("authen/thirdPartyLogin?token=") + third_token + "&companyid=" + companyId;
	if (scene != NULL)
	{
		request->url += string("&scene=") + scene;
	}

	if (phone != NULL)
	{
		request->url += string("&phone=") + phone;
	}

	if (smsCode != NULL)
	{
		request->url += string("&smsCode=") + smsCode;
	}

	if (extend != NULL)
	{
		request->url += string("&extend=") + extend;
	}

	if (szIsLimited != NULL) {
		request->url += string("&islimited=") + szIsLimited;
	}

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

void RequestProcess::SetReqParams(map<string, string>* mapReqParams)
{
	m_mapReqParams.clear();
	m_mapReqParams = *mapReqParams;
}

void RequestProcess::SetParamToUrl(string& url, map<string, string>* mapReqParams)
{
	if (mapReqParams == NULL) return;

	for (map<string, string>::iterator it = mapReqParams->begin(); it != mapReqParams->end(); ++it)
	{
		if (it->first.length() <= 0 || it->second.length() <= 0)
		{
			continue;
		}

		if (url.length() > 0 && url[url.length() - 1] != '?') url += "&";

		url += it->first + "=" + it->second;
	}
}

HttpRequest* RequestProcess::GetClientConfig()
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetClientConfig;
	//添加无效参数logintype规范https/http协议
	request->url = string("/authen/v2/getSystemConfig?logintype=godown");

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGoDownConfigInit(const char* account, const char* isVoice, const char* flowid)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_NewVersionGoDownConfigInit;
	std::string temp = UrlEncoder::encode_go_down(account);
	request->url = string("/authen/v2/safePhoneSmsLogin/init?inputUserId=") + temp.c_str() + "&flowId=" + flowid + "&isVoice=" + isVoice;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGoDownSendSmsCheckCode(const char* captchaInfo, const char* flowid)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_NewVersionGoDownSendSmsCheckCode;
	request->url = string("/authen/v2/safePhoneSmsLogin/verifyCaptcha?captchaInfo=") + captchaInfo + "&flowId=" + flowid;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGoDownconfirmSendSms(const char* flowid, int isVoice)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_NewVersionGoDownconfirmSendSms;
	request->url = string("/authen/v2/safePhoneSmsLogin/confirmSend?flowId=") + flowid + "&isVoice=" + IntToStr(isVoice);

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGoDownconfirmLogin(const char* account, const char* flowid, const char* verifyCode, int keepLoginFlag)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_NewVersionGoDownconfirmLogin;
	std::string temp = UrlEncoder::encode_go_down(account);
	request->url = string("/authen/v2/safePhoneSmsLogin/confirmLogin?flowId=") + flowid + "&verifyCode=" + verifyCode + "&inputUserId=" + temp.c_str();

	//新增实名登录中的自动登录参数keepLoginFlag 20241009
	char buff[1024];
	sprintf(buff, "&keepLoginFlag=%d", keepLoginFlag);
	request->url += buff;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetFastLogin(const char* autoLoginKey)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_NewVersionFastLogin;
	request->url = string("authen/v2/fastInLogin?keepLoginKey=") + autoLoginKey;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetTicketNew()
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_NewVersionGetTicket;
	request->url = string("authen/v2/getTicket?tgt=") + m_tgt;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::FastLoginActiveDeleteAccount(const char* keepLoginKey, const char* inputUserId)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_FastLoginActiveDeleteAccount;
	request->url = string("authen/v2/deleteKeepLoginKey?keepLoginKey=") + keepLoginKey + "&inputUserId=" + inputUserId;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::CheckAccountLoginType(const char* tgt)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CheckLoginTypeAccount;
	request->url = string("authen/getAccountInfo.json?tgt=") + tgt;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::CheckCalculate(const char* star_sign, const char* date)
{
	TRACET();
	//https://apis.tianapi.com/star/index?key=APIKEY&astro=taurus
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CheckCalculate;
	if (strlen(date) > 0)
	{
		request->url = string("star/index?key=") + "0ddc99d74fa2bab8e15971f7b531266b" + "&astro=" + UrlEncoder::StarUrlEncode(GbkToUtf8(star_sign)) + "&date=" + date;
	}
	else
	{
		request->url = string("star/index?key=") + " 0ddc99d74fa2bab8e15971f7b531266b" + "&astro=" + UrlEncoder::StarUrlEncode(GbkToUtf8(star_sign));
	}


	SetParamToUrl(request->url, &m_mapReqParams);

	SetCalculateCommonParam(request);
	return request;

}

HttpRequest* RequestProcess::CheckInitAdv()
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_CheckInitAdv;
	request->url = "/adi/client/init";

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	std::string extendInfo;
	if (m_deviceId.size() > 0)
	{
		extendInfo = ConstructInitAdvJson(m_deviceId);
	}
	std::string extendInfo_encode = UrlEncoder::encode_go_down(extendInfo.c_str());

	request->postData = std::string("appId=") + ConvertStringFromInt(m_appid) +
		"&epOs=" + "windows" +
		"&epMode=" + "native" +
		"&extendInfo=" + extendInfo_encode;

	SetParamToUrl(request->url, &m_mapReqParams);
	SetAdvCommonParam(request, false);

	return request;
}

HttpRequest* RequestProcess::GetGuardDianSendSms(const char* phone, int isVoice, const char* flowId)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GuardDianSendSms;
	request->url = string("authen/v2/tutor/sendSms?phone=") + UrlEncoder::encode_go_down(phone) + "&flowId=" + flowId + "&isVoice=" + IntToStr(isVoice);

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGuardDianSendSmsCheckCode(const char* captchaInfo, int isVoice, const char* flowId)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GuardDianSendSmsCheckCode;
	request->url = string("/authen/v2/tutor/verifyCaptcha?captchaInfo=")
		+ captchaInfo + "&flowId=" + flowId + "&isVoice=" + IntToStr(isVoice);

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetGuardDianConfrimSendSmsResult(int ageCheckFlag, const char* flowId, const char* verifyCode,
	const char* tutorName, const char* tutorIdCard, const char* phone)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GuardDianConfrimSendSmsResult;

	request->url = string("/authen/v2/tutor/supply?ageCheckFlag=")
		+ IntToStr(ageCheckFlag) + "&flowId=" + flowId + "&verifyCode="
		+ verifyCode + "&tutorName="
		+ UrlEncoder::encode_go_down(GbkToUtf8(tutorName).c_str())
		+ "&tutorIdCard=" + UrlEncoder::encode_go_down(GbkToUtf8(tutorIdCard).c_str())
		+ "&phone=" + UrlEncoder::encode_go_down(phone);

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetFaceRecognitionUrl(const char* confType, const char* confKey)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetFaceRecognitionUrl;

	request->url = string("/api/faceRecognition/getFaceRecognitionUrl?confType=")
		+ confType + "&confKey=" + confKey;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetGetFaceUrlCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetQueryFaceVerifyTicketStatus(const char* appId, const char* ticket)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_QueryFaceVerifyTicketStatus;

	request->url = string("/api/faceRecognition/queryFaceVerifyTicketStatus?appId=")
		+ appId + "&ticket=" + ticket;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetGetFaceUrlCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetFaceHolderRegistrationUrl(const char* confType, const char* confKey)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetFaceHolderRegistrationUrl;

	request->url = string("/api/faceRecognition/getFaceRecognitionUrl?confType=")
		+ confType + "&confKey=" + confKey;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetGetFaceUrlCommonParam(request);
	return request;
}

HttpRequest* RequestProcess::GetActiveCodeLoginResult(const char* activeCode)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GetActiveCodeLoginResult;

	request->url = string("/authen/v2/activationCodeLogin?activationCode=")
		+ UrlEncoder::encode_go_down(activeCode) + "&tgt=" + m_tgt;

	SetParamToUrl(request->url, &m_mapReqParams);

	SetNewVersionCommonParam(request);
	return request;
}

//RequestProcess
//├─ 负责拼 HttpRequest
//├─ 但：加密 / 签名不在这里
//↓
//Wegame3dsCode.dll
//├─ 管理 publicKey
//├─ 管理 randKey / token
//├─ 管理 3DES / RSA
//├─ 管理“是否已握手”

//Gunion-Wegame初始化
HttpRequest* RequestProcess::GetCheckGunionWegameInitRequest(
	const char* appId,
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionWegameInit;
	request->url = string("v1/account/initialize");
	request->postData = encryptedParams;

	SetParamToUrl(request->url, &m_mapReqParams);
	SetCommonParam(request, appId, gunionToken, signature, false);

	return request;
}

//Gunion-Wegame登录
HttpRequest* RequestProcess::GetCheckGunionWegameLoginRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionWegameLogin;
	request->url = string("/v1/cooperation/wegameLogin");
	request->postData = encryptedParams;

	SetParamToUrl(request->url, &m_mapReqParams);
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken, // gunionToken
		signature,   // signature
		false        // needEncrypt
	);

	return request;
}

//Gunion-Wegame 短信发送请求
HttpRequest* RequestProcess::GetCheckGunionWegameSmsSendRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionWegameSmsSend;
	request->url = string("v1/basic/smssend");
	request->postData = encryptedParams;

	SetParamToUrl(request->url, &m_mapReqParams);
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken, // gunionToken
		signature,   // signature
		false        // needEncrypt
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionWegamePicSmsSendRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionWegamePicSmsSend;
	request->url = string("v1/basic/picCheckSmsSend2");
	request->postData = encryptedParams;

	SetParamToUrl(request->url, &m_mapReqParams);
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken, // gunionToken
		signature,   // signature
		false        // needEncrypt
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionWegameThirdAccountBindPhoneRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionWegameThirdAccountBindPhone;
	request->url = string("v1/account/thirdAccountBindPhone");
	request->postData = encryptedParams;

	SetParamToUrl(request->url, &m_mapReqParams);
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken, // gunionToken
		signature,   // signature
		false        // needEncrypt
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionWegameFillRealinfoRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionWegameFillRealinfo;
	request->url = string("v1/account/fillrealinfo");
	request->postData = encryptedParams;

	SetParamToUrl(request->url, &m_mapReqParams);
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken, // gunionToken
		signature,   // signature
		false        // needEncrypt
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionWegameGetTicketRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionWegameGetTicket;
	request->url = "v1/open/getticket2";
	request->postData = encryptedParams;

	SetParamToUrl(request->url, &m_mapReqParams);
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken, // gunionToken
		signature,   // signature
		false        // needEncrypt
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionWegameGetPayTicketRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionWegameGetPayTicket;
	request->url = "v1/open/getticket2";
	request->postData = encryptedParams;

	SetParamToUrl(request->url, &m_mapReqParams);
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken, // gunionToken
		signature,   // signature
		false        // needEncrypt
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckDoPrivateRequest(
	const char* appId,
	const char* scene)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionGetPrivateUrl;

	// ------------------------------------------------------------
	// 使用初始化得到的 appId
	// ------------------------------------------------------------
	std::stringstream ss;
	ss << m_appid;
	std::string str_appid = ss.str();

	// ------------------------------------------------------------
	// 构造 URL（只拼 path + query）
	// host 由 SetCommonParam 决定
	// ------------------------------------------------------------
	request->url = std::string("agreement/all")
		+ "?appId=" + str_appid
		+ "&channelCode=windows"
		+ "&platform=pc_cplus"
		+ "&scene=" + scene
		+ "&version=0";

	// ------------------------------------------------------------
	// GET 请求必须走 SetCommonParam 的 GET 分支
	// 不传 token，不传 signature
	// ------------------------------------------------------------
	SetCommonParam(
		request,
		str_appid.c_str(),
		"",      // 不需要 token
		"",      // 不需要 signature
		true     // isGet = true
	);
	return request;
}

//Gunion 短信发送请求
HttpRequest* RequestProcess::GetCheckGunionSmsSendRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionSmsSend;
	request->url = string("v1/basic/smssend");
	request->postData = encryptedParams;

	SetParamToUrl(request->url, &m_mapReqParams);
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken, // gunionToken
		signature,   // signature
		false        // needEncrypt
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionPicSmsSendRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionPicSmsSend;
	request->url = string("v1/basic/picCheckSmsSend2");
	request->postData = encryptedParams;

	SetParamToUrl(request->url, &m_mapReqParams);
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken, // gunionToken
		signature,   // signature
		false        // needEncrypt
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionSmsLoginRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionSmsLogin;

	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/account/smsLogin";

	// ------------------------------------------------------------
	// 加密后的业务参数直接放入 postData
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 附加公共 URL 参数（如时间戳等）
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// false 表示已加密，不再二次加密
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,   // token
		signature,     // signature
		false          // needEncrypt
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionRealNameRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionRealName;

	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/account/fillrealinfo";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionAutoLoginRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionAutoLogin;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/account/loginauto";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionLogoutRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionLogout;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/account/logout";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionPwdLoginRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionPwdLogin;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/account/login";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionPicPwdLoginRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionPicPwdLogin;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/account/checkCodeLogin";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionThirdLoginRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionThirdLogin;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/account/thirdAccountTicketLogin";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionSpecialPwdLoginRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionSpecialPwdLogin;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/accountUnification/login";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionSpecialPicPwdLoginRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionSpecialPicPwdLogin;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/accountUnification/picCheckLogin";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionLoginAreaRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionSetLoginArea;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/basic/loginarea";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionGetLoginAreaRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionGetLoginArea;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/basic/getAreaList";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionSpecialSmsSendRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionSpecialSmsSend;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/accountUnification/smsSend";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionSpecialPicSmsSendRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionSpecialPicSmsSend;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/accountUnification/picCheckSmsSend";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionSpecialConfirmSmsSendRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionSpecialConfirmSmsSend;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/accountUnification/confirmSmsSend";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionSpecialCheckSmsLoginRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionSpecialSmsLogin;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/accountUnification/smsLogin";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionPayEntranceRequest(
	int iSSandboxAccount,
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionPayEntrance;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/pay/pcentrance";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCheckPayEntraceCommonParam(
		request,
		appidStr.c_str(),
		iSSandboxAccount,
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionCheckPayOrderStatusRequest(
	int iSSandboxAccount,
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionPayOrderStatus;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/pay/orderstatus";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCheckPayEntraceCommonParam(
		request,
		appidStr.c_str(),
		iSSandboxAccount,
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionCheckActivationRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionActivation;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/account/activation";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionSetTutorInfoRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionSetTutorLogin;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/account/setTutorInfo";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGunionWegameChannelLoginRequest(
	const std::string& gunionToken,
	const std::string& encryptedParams,
	const std::string& signature)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionWegameChannelLogin;
	// ------------------------------------------------------------
	// 接口路径
	// ------------------------------------------------------------
	request->url = "v1/account/twiceSelectLogin";

	// ------------------------------------------------------------
	// 加密后的参数
	// ------------------------------------------------------------
	request->postData = encryptedParams;

	// ------------------------------------------------------------
	// 公共 URL 参数
	// ------------------------------------------------------------
	SetParamToUrl(request->url, &m_mapReqParams);

	// ------------------------------------------------------------
	// 设置公共参数（token + signature）
	// ------------------------------------------------------------
	std::string appidStr = std::to_string(m_appid);

	SetCommonParam(
		request,
		appidStr.c_str(),
		gunionToken,
		signature,
		false
	);

	return request;
}

///////////////////////////////////////////////////////////////////////////////
// Gunion 获取 RSA 公钥请求
///////////////////////////////////////////////////////////////////////////////
HttpRequest* RequestProcess::BuildGunionLoginGetPublicKeyRequest()
{
	TRACET();

	// ----------------------
	// 创建请求对象
	// ----------------------
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionLoginGetPublicKey;

	// ----------------------
	// 接口地址
	// ----------------------
	request->url = "v1/basic/publickey";

	// ----------------------
	// 公钥接口不需要加密参数
	// ----------------------
	request->postData.clear();

	SetParamToUrl(request->url, &m_mapReqParams);

	// ----------------------
	// 设置通用参数（appid / channel / platform 等）
	//   - 不需要 signature
	//   - 不需要 token / randKey
	// ----------------------
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		"",          // gunionToken
		"",          // gunionSignature
		false        // methode
	);

	return request;
}

///////////////////////////////////////////////////////////////////////////////
// Gunion 会话密钥协商请求（NegotiateKey）
///////////////////////////////////////////////////////////////////////////////
HttpRequest* RequestProcess::BuildGunionLoginHandshakeRequest(
	const std::string& encryptedRandKey)
{
	TRACET();

	if (encryptedRandKey.empty())
		return nullptr;

	// ----------------------
	// 创建请求对象
	// ----------------------
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionLoginHandshake;

	// ----------------------
	// 接口地址
	// ----------------------
	request->url = "v1/basic/handshake";

	// ----------------------
	// 设置 POST 数据
	//   - 内容为 RSA 加密后的 randKey
	// ----------------------
	request->postData = encryptedRandKey;

	// ----------------------
	// 设置通用参数
	//   - 不需要 signature
	//   - 不携带 token（握手阶段尚未建立会话）
	// ----------------------
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		"",          // gunionToken
		"",          // gunionSignature
		false        // methode
	);

	return request;
}

HttpRequest* RequestProcess::GetCheckGameAgreementUrlContent(
	const std::string& appId,
	const std::string& scene)
{
	TRACET();

	HttpRequest* request = new HttpRequest;
	if (!request)
		return nullptr;

	request->requestCode = REQ_GunionGetAgreementContent;

	// ------------------------------------------------------------
	// 使用初始化得到的 appId
	// ------------------------------------------------------------
	std::stringstream ss;
	ss << m_appid;
	std::string str_appid = ss.str();

	// ------------------------------------------------------------
	// 构造 URL（只拼 path + query）
	// host 由 SetCommonParam 决定
	// ------------------------------------------------------------
	request->url = std::string("agreement/all")
		+ "?appId=" + str_appid
		+ "&channelCode=windows"
		+ "&platform=pc_cplus"
		+ "&scene=" + scene
		+ "&version=0";

	// ------------------------------------------------------------
	// GET 请求必须走 SetCommonParam 的 GET 分支
	// 不传 token，不传 signature
	// ------------------------------------------------------------
	SetCommonParam(
		request,
		str_appid.c_str(),
		"",      // 不需要 token
		"",      // 不需要 signature
		true     // isGet = true
	);
	return request;
}

///////////////////////////////////////////////////////////////////////////////
// Gunion 获取 RSA 公钥请求
///////////////////////////////////////////////////////////////////////////////
HttpRequest* RequestProcess::BuildGunionGetPublicKeyRequest()
{
	TRACET();

	// ----------------------
	// 创建请求对象
	// ----------------------
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionGetPublicKey;

	// ----------------------
	// 接口地址
	// ----------------------
	request->url = "v1/basic/publickey";

	// ----------------------
	// 公钥接口不需要加密参数
	// ----------------------
	request->postData.clear();

	SetParamToUrl(request->url, &m_mapReqParams);

	// ----------------------
	// 设置通用参数（appid / channel / platform 等）
	//   - 不需要 signature
	//   - 不需要 token / randKey
	// ----------------------
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		"",          // gunionToken
		"",          // gunionSignature
		false        // methode
	);

	return request;
}

///////////////////////////////////////////////////////////////////////////////
// Gunion-WeGame 会话密钥协商请求（NegotiateKey）
///////////////////////////////////////////////////////////////////////////////
HttpRequest* RequestProcess::BuildGunionHandshakeRequest(
	const std::string& encryptedRandKey)
{
	TRACET();

	if (encryptedRandKey.empty())
		return nullptr;

	// ----------------------
	// 创建请求对象
	// ----------------------
	HttpRequest* request = new HttpRequest;
	request->requestCode = REQ_GunionHandshake;

	// ----------------------
	// 接口地址
	// ----------------------
	request->url = "v1/basic/handshake";

	// ----------------------
	// 设置 POST 数据
	//   - 内容为 RSA 加密后的 randKey
	// ----------------------
	request->postData = encryptedRandKey;

	// ----------------------
	// 设置通用参数
	//   - 不需要 signature
	//   - 不携带 token（握手阶段尚未建立会话）
	// ----------------------
	std::string appidStr = std::to_string(m_appid);
	SetCommonParam(
		request,
		appidStr.c_str(),
		"",          // gunionToken
		"",          // gunionSignature
		false        // methode
	);

	return request;
}

void RequestProcess::SetNewVersionCommonParam(HttpRequest* request, bool isGet)
{
	request->hostName = "n.cas.sdo.com";
	request->port = m_port;
	request->hostName2 = "n.cas.sdo.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;

	string* strurl;
	if (isGet)
	{
		request->method = "GET";
		strurl = &request->url;
	}
	else
	{
		request->method = "POST";
		strurl = &request->postData;
	}

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	if (m_macId.size() == 0) {
		char szTemp[128] = { 0 };
		int nTempSize = 128;
		CComputerInfo::GetMacAddress(szTemp, nTempSize);
		m_macId = szTemp;
	}

	if (m_IpId.size() == 0)
	{
		char szIpTemp[128] = { 0 };
		int nIpTempSize = 128;
		CComputerInfo::GetIPAddress(szIpTemp, nIpTempSize);
		m_IpId = szIpTemp;
	}

	//计算机名称
	std::wstring wstring_computerName;
	if (GetComputerNameInterface(wstring_computerName))
	{

	}
	std::string string_computer = LocalUnicodeToUtf8(wstring_computerName);
	std::string encodedName = UrlEncoder::encode_go_down(string_computer.c_str());

	std::string extendInfo;
	if (adTraceId.size() > 0)
	{
		extendInfo = ConstructJson(adTraceId);
	}
	std::string extendInfo_encode = UrlEncoder::encode_go_down(extendInfo.c_str());

	char buff[1024];
	if (m_appid == 88 || m_appid == 11 || m_appid == 791000810)
	{
		sprintf(buff, "&authenSource=1&appId=%d&areaId=%d&appIdSite=%d&locale=%s&productId=%d&frameType=1&endpointOS=1&version=21&customSecurityLevel=%d&deviceId=%s&thirdLoginExtern=%s&macId=%s&epIp=%s&epName=%s&extendInfo=%s&sdoVersion=%s&runTimeId=%s&channelId=%s",
			m_appid, m_areaid, m_appid, m_locale != 2 ? "zh_CN" : "en_US", m_productId, m_customSecurityLevel, m_deviceId.c_str(), LoginClient::getThirdLoginExtern().c_str(), m_macId.c_str(), m_IpId.c_str(), encodedName.c_str(), extendInfo_encode.c_str(), sdoVersion.c_str(), sdoRunTimerId.c_str(), LoginClient::getChannelIdExtern().c_str());
	}
	else
	{
		sprintf(buff, "&authenSource=1&appId=%d&areaId=%d&appIdSite=%d&locale=%s&productId=%d&frameType=1&endpointOS=1&version=21&customSecurityLevel=%d&deviceId=%s&thirdLoginExtern=%s&macId=%s&epIp=%s&epName=%s&extendInfo=%s&sdoVersion=%s&runTimeId=%s&channelId=%s",
			m_appid, m_areaid, m_appid, m_locale != 2 ? "zh_CN" : "en_US", m_productId, m_customSecurityLevel, m_deviceId.c_str(), LoginClient::getThirdLoginExtern().c_str(), m_macId.c_str(), m_IpId.c_str(), encodedName.c_str(), extendInfo_encode.c_str(), sdoVersion.c_str(), sdoRunTimerId.c_str(), LoginClient::getChannelIdExtern().c_str());
	}
	*strurl += buff;
	if (!m_productVersion.empty())
	{
		*strurl += "&productVersion=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}
	if (m_tag != -1)
	{
		sprintf(buff, "&tag=%d", m_tag);
		*strurl += buff;
	}

	//MessageBoxA(NULL,(*strurl).c_str(),"strurl",0);
}

std::string RequestProcess::GenerateUniqueId()
{
	// 获取当前时间戳
	std::time_t now = std::time(0);

	// 获取精确到毫秒的系统时间
	SYSTEMTIME st;
	GetSystemTime(&st);

	// 创建一个字符串流，用于拼接 ID
	std::stringstream ss;

	// 将时间戳转换为十六进制表示
	ss << std::hex << now;

	// 加入毫秒部分，确保唯一性
	ss << std::hex << st.wMilliseconds;

	// 加入一个随机数，确保即使在同一时间点生成的ID也不一样
	ss << std::hex << (rand() % 0xFFFF);

	// 加入MAC地址和IP地址
	ss << m_macId;
	ss << m_IpId;

	// 返回生成的唯一 ID
	return ss.str();
}

std::string RequestProcess::GunionIntToString(int number)
{
	// 使用 stringstream 进行转换
	std::stringstream ss;
	ss << number;
	return ss.str();
}

//////////////////////////////////////////////////////////////////////////
// 

struct TlvData
{
	unsigned short type;
	unsigned short length;
	char value[0];
};

int ResponseProcess::Process(LoginClient* loginClient, int result, int requestCode,
	const string& response, const vector<string>& vecCookies,
	map<string, string>& keyValues)
{
	std::string errorMsg;

	if (requestCode == REQ_GunionWegameInit
		|| requestCode == REQ_GunionWegameLogin
		|| requestCode == REQ_GunionWegameSmsSend
		|| requestCode == REQ_GunionWegamePicSmsSend
		|| requestCode == REQ_GunionWegameThirdAccountBindPhone
		|| requestCode == REQ_GunionWegameInit
		|| requestCode == REQ_GunionWegameFillRealinfo
		|| requestCode == REQ_GunionWegameGetTicket
		|| requestCode == REQ_GunionWegameGetPayTicket
		|| requestCode == REQ_GunionGetPublicKey
		|| requestCode == REQ_GunionHandshake
		|| requestCode == REQ_GunionSmsSend
		|| requestCode == REQ_GunionPicSmsSend
		|| requestCode == REQ_GunionSmsLogin
		|| requestCode == REQ_GunionSpecialSmsSend
		|| requestCode == REQ_GunionSpecialPicSmsSend
		|| requestCode == REQ_GunionSpecialConfirmSmsSend
		|| requestCode == REQ_GunionSpecialSmsLogin
		|| requestCode == REQ_GunionPwdLogin
		|| requestCode == REQ_GunionPicPwdLogin
		|| requestCode == REQ_GunionSpecialPwdLogin
		|| requestCode == REQ_GunionSpecialPicPwdLogin
		|| requestCode == REQ_GunionThirdLogin
		|| requestCode == REQ_GunionAutoLogin
		|| requestCode == REQ_GunionLogout
		|| requestCode == REQ_GunionRealName
		|| requestCode == REQ_GunionGetLoginArea
		|| requestCode == REQ_GunionSetLoginArea
		|| requestCode == REQ_GunionPayEntrance
		|| requestCode == REQ_GunionPayOrderStatus
		|| requestCode == REQ_GunionActivation
		|| requestCode == REQ_GunionSetTutorLogin
		|| requestCode == REQ_GunionWegameChannelLogin
		|| requestCode == REQ_GunionLoginGetPublicKey
		|| requestCode == REQ_GunionLoginHandshake)
	{
		if (result != 0)
		{
			keyValues.insert(make_pair("code", IntToStr(ERROR_REQUEST_FIRST + result)));
			keyValues.insert(make_pair("msg", GbkToUtf8("网络传输异常!")));
			return 0;
		}
	}
	else
	{
		if (result != 0)
		{
			keyValues.insert(make_pair("resultCode", IntToStr(ERROR_REQUEST_FIRST + result)));
			keyValues.insert(make_pair("failReason", GbkToUtf8("网络传输异常!")));
			return 0;
		}
	}

	for (vector<string>::const_iterator iter = vecCookies.begin();
		iter != vecCookies.end(); ++iter)
	{
		const string& str = *iter;
		size_t pos = str.find("=");
		if (pos != string::npos)
		{
			string name = str.substr(0, pos);
			string value = str.substr(pos + 1);
			pos = value.find(";");
			if (pos != string::npos)
				value = value.substr(0, pos);
			if (name == "CASTGC")
				keyValues.insert(make_pair("tgt", value));
			else if (name == "CAS_AUTO_LOGIN")
				keyValues.insert(make_pair("autoLoginSessionKey", value));
			else if (name == "CODEKEY")
				keyValues.insert(make_pair("codeKey", value));
			else if (name == "CAS_PUSHMSG_SESSIONKEY")
				keyValues.insert(make_pair("pushMsgSessionKey", value));
			else
				keyValues.insert(make_pair(name, value));
		}
	}

	if (requestCode == REQ_GetQrCode)
	{
		if (keyValues.find("codeKey") == keyValues.end())
		{
			keyValues.insert(make_pair("resultCode", IntToStr(ERROR_RESPONSE)));
			return 0;
		}
		keyValues.insert(make_pair("resultCode", "0"));
		keyValues.insert(make_pair("picData", response));
	}
	else if (requestCode == REQ_CreateWeGameOrder
		|| requestCode == REQ_CreateQQGameOrder
		|| requestCode == REQ_CreateLxOrder
		|| requestCode == REQ_CreateSteamChannelOrder
		|| requestCode == REQ_CreateCQ4Order
		|| requestCode == REQ_CreateCQ4Query)
	{
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;
		if (atoi(value) == 0) {
			json_object* data = json_object_object_get(root, "data");
			if (data != NULL && !is_error(data)) {
				json_object_object_foreach(data, key, val)
				{
					if (val != NULL && !is_error(val)) {
						keyValues[key] = json_object_get_string(val);
					}
					else {
						keyValues[key] = "";
					}
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["resultMsg"] = value;
			}
		}
		json_object_put(root);
	}
	else if (requestCode == REQ_FaceVerifyInit
		|| requestCode == REQ_FaceCodeResult
		|| requestCode == REQ_FaceSendAction)
	{
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "resultCode");
		if (is_error(json))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;
		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data)) {
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val)) {
					keyValues[key] = json_object_get_string(val);
				}
				else {
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("resultMsg") == keyValues.end())
		{
			json = json_object_object_get(root, "resultMsg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["resultMsg"] = value;
			}
		}

		json_object_put(root);

	}
	else if (requestCode == REQ_GetClientConfig)
	{
		//获取登录器功能相关配置
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			keyValues["config"] = json_object_get_string(data);
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_NewVersionGoDownConfigInit)
	{
		//下行短信相关账号配置的获取
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_NewVersionGoDownSendSmsCheckCode)
	{
		//下行短信带图验验证
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_NewVersionGoDownconfirmSendSms)
	{
		//新版下行短信确认发送
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_NewVersionGoDownconfirmLogin)
	{
		//新版下行短信登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_NewVersionFastLogin)
	{
		//新版快速登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_NewVersionGetTicket)
	{
		//新版获取票据
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_FastLoginActiveDeleteAccount)
	{
		//快速登录主动删除账号
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data) && json_object_get_type(data) == json_type_object)
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val)) {
					keyValues[key] = json_object_get_string(val);
				}
				else {
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_NewVersionGetTicket)
	{
		//新版获取票据
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_CheckLoginTypeAccount)
	{
		//快速登录主动删除账号
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_CheckCalculate)
	{
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "result");
		if (data != NULL && !is_error(data))
		{
			keyValues["config"] = json_object_get_string(data);
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_CheckInitAdv)
	{
		//广告系统初始化接口
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "resultCode");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		/*
			{
			"resultCode": -10906994,
			"resultMsg": "缺失必填请求参数",
			"data": "请求参数缺失:appId"
			}
		*/
		//在这种情况下，data 是一个字符串，而不是对象。json_object_object_foreach 只能遍历对象类型，无法遍历字符串，导致问题发生。
		json_object* data = json_object_object_get(root, "data");
		if (data != NULL)
		{
			// 仅在 data 是对象类型时进行遍历
			if (json_object_get_type(data) == json_type_object)
			{
				json_object_object_foreach(data, key, val)
				{
					if (val != NULL && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
			}
			else
			{
				// data 不是对象，直接作为字符串处理
				keyValues["data"] = json_object_get_string(data);
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "resultMsg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GuardDianSendSms)
	{
		//监护人信息补填短信发送
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GuardDianSendSmsCheckCode)
	{
		//监护人信息补填带图验短信发送
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GuardDianConfrimSendSmsResult)
	{
		//监护人信息补填短信确认
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GetActiveCodeLoginResult)
	{
		//激活码登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionGetPublicKey)
	{
		// Gunion 获取公钥
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value;

		if (atoi(value) == 0)
		{
			// data 是一个 object，不是加密字符串
			json_object* data = json_object_object_get(root, "data");
			if (data && !is_error(data))
			{
				json_object* keyJson =
					json_object_object_get(data, "key");
				if (keyJson && !is_error(keyJson))
				{
					const char* key = json_object_get_string(keyJson);
					if (key)
					{
						// 这里把 key 交给上层（或直接由 LoginClient 使用）
						keyValues["key"] = key;
					}
				}
			}
		}

		if (keyValues["code"] != "0" &&
			keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["msg"] = value ? value : "";
			}
		}
		json_object_put(root);
	}
	else if (requestCode == REQ_GunionHandshake)
	{
		// Gunion 握手（建立会话 / 获取 token）
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value;

		if (atoi(value) == 0)
		{
			// data 是 3DES 加密后的 base64 字符串
			json_object* dataJson = json_object_object_get(root, "data");
			const char* encryptedData =
				dataJson ? json_object_get_string(dataJson) : nullptr;

			if (encryptedData && *encryptedData)
			{
				std::string decrypted;

				// 新架构：统一走 LoginClient
				if (loginClient->DecryptGunion3Des(encryptedData, decrypted))
				{
					json_object* dataRoot =
						json_tokener_parse(decrypted.c_str());

					if (dataRoot && !is_error(dataRoot))
					{
						json_object* tokenJson =
							json_object_object_get(dataRoot, "token");
						if (tokenJson && !is_error(tokenJson))
						{
							keyValues["token"] =
								json_object_get_string(tokenJson);
						}
						json_object_put(dataRoot);
					}
				}
				else
				{
					// 解密失败 → 协议失败
					keyValues["code"] = IntToStr(ERROR_RESPONSE);
				}
			}
		}

		if (keyValues["code"] != "0" &&
			keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				errorMsg = ConvertUcs2ToAnsi(value);
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionWegameInit)
	{
		//GunionWegame响应 Gunion初始化
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["code"] = value;

		if (atoi(value) == 0)
		{
			json_object* encrypted_data_json =
				json_object_object_get(root, "data");

			const char* encrypted_data =
				encrypted_data_json ? json_object_get_string(encrypted_data_json) : nullptr;

			if (!encrypted_data || !*encrypted_data)
			{
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			// 新架构：3DES 解密（不再返回 char*）
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 解密失败 → 协议失败
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			// 只解析 std::string 解密结果
			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		if (keyValues.find("code") == keyValues.end())
		{
			keyValues["code"] = "0";
		}

		if (keyValues["code"] != "0" && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				errorMsg = ConvertUcs2ToAnsi(value);
				keyValues["msg"] = errorMsg;
			}
		}
		json_object_put(root);
	}
	else if (requestCode == REQ_GunionWegameLogin)
	{
		//GunionWegame响应 Wegame登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["code"] = value;

		if (atoi(value) == 0)
		{
			json_object* encrypted_data_json =
				json_object_object_get(root, "data");

			const char* encrypted_data =
				encrypted_data_json ? json_object_get_string(encrypted_data_json) : nullptr;

			if (!encrypted_data || !*encrypted_data)
			{
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			// 新架构：3DES 解密（不再返回 char*）
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 解密失败 → 协议失败
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			// 只解析 std::string 解密结果
			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		if (keyValues.find("code") == keyValues.end())
		{
			keyValues["code"] = "0";
		}

		if (keyValues["code"] != "0" && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				errorMsg = ConvertUcs2ToAnsi(value);
				keyValues["msg"] = errorMsg;
			}
		}
		json_object_put(root);
	}
	else if (requestCode == REQ_GunionWegameSmsSend)
	{
		//GunionWegame短信发送
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["code"] = value;

		if (atoi(value) == 0)
		{
			json_object* encrypted_data_json =
				json_object_object_get(root, "data");

			const char* encrypted_data =
				encrypted_data_json ? json_object_get_string(encrypted_data_json) : nullptr;

			if (!encrypted_data || !*encrypted_data)
			{
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			// 新架构：3DES 解密（不再返回 char*）
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 解密失败 → 协议失败
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			// 只解析 std::string 解密结果
			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		if (keyValues.find("code") == keyValues.end())
		{
			keyValues["code"] = "0";
		}

		if (keyValues["code"] != "0" && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				errorMsg = ConvertUcs2ToAnsi(value);
				keyValues["msg"] = errorMsg;
			}
		}
		json_object_put(root);
	}
	else if (requestCode == REQ_GunionWegamePicSmsSend)
	{
		//GunionWegame图验短信发送
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（仅表示业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不再依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				json_object* data =
					json_tokener_parse(decrypted.c_str());

				if (data && !is_error(data))
				{
					json_object_object_foreach(data, key, val)
					{
						if (val && !is_error(val))
						{
							keyValues[key] = json_object_get_string(val);
						}
						else
						{
							keyValues[key] = "";
						}
					}
					json_object_put(data);
				}
			}
			else
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}
		}

		// ---------- 3. 失败但 data 没给 msg，则从 root.msg 兜底 ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionWegameThirdAccountBindPhone)
	{
		//GunionWegame绑定官方账号
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（仅表示业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 没给） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionWegameFillRealinfo)
	{
		// GunionWegame：实名认证绑定
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			// 新架构：3DES 解密
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			// 解析解密后的 JSON
			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionWegameGetTicket)
	{
		// Gunion 响应：获取 Ticket
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionWegameGetPayTicket)
	{
		// GunionWegame 响应：获取 支付Ticket
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionLoginGetPublicKey)
	{
		// GunionLogin 获取公钥
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value;

		if (atoi(value) == 0)
		{
			// data 是一个 object，不是加密字符串
			json_object* data = json_object_object_get(root, "data");
			if (data && !is_error(data))
			{
				json_object* keyJson =
					json_object_object_get(data, "key");
				if (keyJson && !is_error(keyJson))
				{
					const char* key = json_object_get_string(keyJson);
					if (key)
					{
						// 这里把 key 交给上层（或直接由 LoginClient 使用）
						keyValues["key"] = key;
					}
				}
			}
		}

		if (keyValues["code"] != "0" &&
			keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["msg"] = value ? value : "";
			}
		}
		json_object_put(root);
	}
	else if (requestCode == REQ_GunionLoginHandshake)
	{
		// GunionLogin 握手（建立会话 / 获取 token）
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value;

		if (atoi(value) == 0)
		{
			// data 是 3DES 加密后的 base64 字符串
			json_object* dataJson = json_object_object_get(root, "data");
			const char* encryptedData =
				dataJson ? json_object_get_string(dataJson) : nullptr;

			if (encryptedData && *encryptedData)
			{
				std::string decrypted;

				// 新架构：统一走 LoginClient
				if (loginClient->DecryptGunion3Des(encryptedData, decrypted))
				{
					json_object* dataRoot =
						json_tokener_parse(decrypted.c_str());

					if (dataRoot && !is_error(dataRoot))
					{
						json_object* tokenJson =
							json_object_object_get(dataRoot, "token");
						if (tokenJson && !is_error(tokenJson))
						{
							keyValues["token"] =
								json_object_get_string(tokenJson);
						}
						json_object_put(dataRoot);
					}
				}
				else
				{
					// 解密失败 → 协议失败
					keyValues["code"] = IntToStr(ERROR_RESPONSE);
				}
			}
		}

		if (keyValues["code"] != "0" &&
			keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				errorMsg = ConvertUcs2ToAnsi(value);
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionGetPrivateUrl)
	{
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json))
		{
			keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["return_code"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("return_code") == keyValues.end())
		{
			keyValues["return_code"] = "0";
		}

		if (keyValues["return_code"] != "0" && keyValues.find("return_message") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["return_message"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionGetAgreementContent)
	{
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json))
		{
			keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["return_code"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("return_code") == keyValues.end())
		{
			keyValues["return_code"] = "0";
		}

		if (keyValues["return_code"] != "0" && keyValues.find("return_message") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["return_message"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionSmsSend)
	{
		//Gunion短信发送
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["code"] = value;

		if (atoi(value) == 0)
		{
			json_object* encrypted_data_json =
				json_object_object_get(root, "data");

			const char* encrypted_data =
				encrypted_data_json ? json_object_get_string(encrypted_data_json) : nullptr;

			if (!encrypted_data || !*encrypted_data)
			{
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			// 新架构：3DES 解密（不再返回 char*）
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 解密失败 → 协议失败
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			// 只解析 std::string 解密结果
			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		if (keyValues.find("code") == keyValues.end())
		{
			keyValues["code"] = "0";
		}

		if (keyValues["code"] != "0" && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				errorMsg = ConvertUcs2ToAnsi(value);
				keyValues["msg"] = errorMsg;
			}
		}
		json_object_put(root);
	}
	else if (requestCode == REQ_GunionPicSmsSend)
	{
		//Gunion图验短信发送
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（仅表示业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不再依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				json_object* data =
					json_tokener_parse(decrypted.c_str());

				if (data && !is_error(data))
				{
					json_object_object_foreach(data, key, val)
					{
						if (val && !is_error(val))
						{
							keyValues[key] = json_object_get_string(val);
						}
						else
						{
							keyValues[key] = "";
						}
					}
					json_object_put(data);
				}
			}
			else
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}
		}

		// ---------- 3. 失败但 data 没给 msg，则从 root.msg 兜底 ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionSmsLogin)
	{
		// Gunion 响应：获取 offcial账号短信登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionSpecialSmsSend)
	{
		// Gunion 响应：officalpt账号短信发送
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionSpecialPicSmsSend)
	{
		// Gunion 响应：officalpt账号图验短信发送
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionSpecialConfirmSmsSend)
	{
		// Gunion 响应：officalpt账号短信确认发送
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionSpecialSmsLogin)
	{
		// Gunion 响应：officalpt账号短信登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionPwdLogin)
	{
		// Gunion 响应：offical账号密码登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionPicPwdLogin)
	{
		// Gunion 响应：offical账号图验密码登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionSpecialPwdLogin)
	{
		// Gunion 响应：officalpt账号密码登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionSpecialPicPwdLogin)
	{
		// Gunion 响应：officalpt账号图验密码登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionThirdLogin)
	{
		// Gunion 响应：三方登录QQ/WX/WB
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionAutoLogin)
	{
		// Gunion 响应：自动登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionLogout)
	{
		// Gunion 响应：登出
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionRealName)
	{
		// Gunion 响应：实名认证
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionGetLoginArea)
	{
		// Gunion 响应：获取区服列表
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* data_json = json_object_object_get(root, "data");

		if (data_json && !is_error(data_json))
		{
			json_type t = json_object_get_type(data_json);

			if (t == json_type_string)
			{
				// data 是加密串
				const char* encrypted_data = json_object_get_string(data_json);
				if (encrypted_data && *encrypted_data)
				{
					std::string decrypted;
					if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
					{
						keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
						json_object_put(root);
						return 0;
					}

					json_object* data = json_tokener_parse(decrypted.c_str());
					if (data && !is_error(data))
					{
						json_object_object_foreach(data, key, val)
						{
							keyValues[key] = (val && !is_error(val)) ? json_object_get_string(val) : "";
						}
						json_object_put(data);
					}
				}
			}
			else if (t == json_type_object)
			{
				// data 是明文对象：直接取
				json_object_object_foreach(data_json, key, val)
				{
					keyValues[key] = (val && !is_error(val)) ? json_object_get_string(val) : "";
				}
			}
			else
			{
				// data 是 [] / null / 其他：不处理
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionSetLoginArea)
	{
		// Gunion 响应：设置区服列表
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* data_json = json_object_object_get(root, "data");

		if (data_json && !is_error(data_json))
		{
			json_type t = json_object_get_type(data_json);

			if (t == json_type_string)
			{
				// data 是加密串
				const char* encrypted_data = json_object_get_string(data_json);
				if (encrypted_data && *encrypted_data)
				{
					std::string decrypted;
					if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
					{
						keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
						json_object_put(root);
						return 0;
					}

					json_object* data = json_tokener_parse(decrypted.c_str());
					if (data && !is_error(data))
					{
						json_object_object_foreach(data, key, val)
						{
							keyValues[key] = (val && !is_error(val)) ? json_object_get_string(val) : "";
						}
						json_object_put(data);
					}
				}
			}
			else if (t == json_type_object)
			{
				// data 是明文对象：直接取
				json_object_object_foreach(data_json, key, val)
				{
					keyValues[key] = (val && !is_error(val)) ? json_object_get_string(val) : "";
				}
			}
			else
			{
				// data 是 [] / null / 其他：不处理
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionPayEntrance)
	{
		// Gunion 响应：创建支付订单
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionPayOrderStatus)
	{
		// Gunion 响应：查看支付订单状态
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionActivation)
	{
		// Gunion 响应：激活码登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionSetTutorLogin)
	{
		// Gunion 响应：监护人登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GunionWegameChannelLogin)
	{
		// Gunion 响应：Wegame账号绑定登录
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}

		// ---------- 1. 取 code（业务语义） ----------
		json_object* json = json_object_object_get(root, "code");
		if (is_error(json))
		{
			keyValues["code"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}

		const char* value = json_object_get_string(json);
		keyValues["code"] = value ? value : "0";
		int code = atoi(keyValues["code"].c_str());

		// ---------- 2. 只要存在 data，就尝试解密（不依赖 code） ----------
		json_object* encrypted_data_json =
			json_object_object_get(root, "data");

		const char* encrypted_data = nullptr;

		if (encrypted_data_json &&
			json_object_is_type(encrypted_data_json, json_type_string))
		{
			encrypted_data = json_object_get_string(encrypted_data_json);
		}

		if (encrypted_data && *encrypted_data)
		{
			std::string decrypted;
			if (!loginClient->DecryptGunion3Des(encrypted_data, decrypted))
			{
				// 有 data 但解密失败：协议异常
				keyValues["return_code"] = IntToStr(ERROR_RESPONSE);
				json_object_put(root);
				return 0;
			}

			json_object* data =
				json_tokener_parse(decrypted.c_str());

			if (data && !is_error(data))
			{
				json_object_object_foreach(data, key, val)
				{
					if (val && !is_error(val))
					{
						keyValues[key] = json_object_get_string(val);
					}
					else
					{
						keyValues[key] = "";
					}
				}
				json_object_put(data);
			}
		}

		// ---------- 3. 失败兜底 msg（仅当 data 未提供） ----------
		if (code != 0 && keyValues.find("msg") == keyValues.end())
		{
			json = json_object_object_get(root, "msg");
			if (!is_error(json))
			{
				const char* msg = json_object_get_string(json);
				errorMsg = msg ? ConvertUcs2ToAnsi(msg) : "";
				keyValues["msg"] = errorMsg;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GetFaceRecognitionUrl)
	{
		//人脸初始化返回url
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "resultCode");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "resultMsg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_QueryFaceVerifyTicketStatus)
	{
		// 人身核验: 验证人脸票据验证
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "resultCode");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("resultMsg") == keyValues.end())
		{
			json = json_object_object_get(root, "resultMsg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["resultMsg"] = value;
			}
		}

		json_object_put(root);
	}
	else if (requestCode == REQ_GetFaceHolderRegistrationUrl)
	{
		//人身核验: 信息收集返回url
		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "resultCode");
		if (is_error(json) || json == NULL)
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		if (data != NULL && !is_error(data))
		{
			json_object_object_foreach(data, key, val)
			{
				if (val != NULL && !is_error(val))
				{
					keyValues[key] = json_object_get_string(val);
				}
				else
				{
					keyValues[key] = "";
				}
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "resultMsg");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	else
	{
		// 由于采用cookie方式自动登录只能支持一个帐号登录，并且会和网页自动登录发生冲突
		// 因此除了二维码之外所有的参数改为通过响应包内容直接获取
		// 这里为了兼容老的方式仍然保留cookie参数的逻辑，
		// 因此如果响应包中包含的参数需要覆盖cookie中获取的参数，
		// 因此这里不能使用map::insert，只能使用map[key]=value方式

		json_object* root = json_tokener_parse(response.c_str());
		if (is_error(root))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			return 0;
		}
		json_object* json = json_object_object_get(root, "return_code");
		if (is_error(json))
		{
			keyValues["resultCode"] = IntToStr(ERROR_RESPONSE);
			json_object_put(root);
			return 0;
		}
		const char* value = json_object_get_string(json);
		keyValues["resultCode"] = value;

		json_object* data = json_object_object_get(root, "data");
		json_object_object_foreach(data, key, val)
		{
			//新增极验解析wangwenhu 20210607
			if (strcmp(key, "captchaParams") == 0) {
				json_object* obj = json_tokener_parse(json_object_get_string(val));
				if (obj != NULL && !is_error(obj)) {
					json_object* gtData = json_object_object_get(obj, "gtData");
					if (gtData != NULL && !is_error(gtData)) {
						json_object_object_foreach(gtData, gkey, value) {
							keyValues[gkey] = json_object_get_string(value);
						}
					}
					json_object* picUrl = json_object_object_get(obj, "picUrl");
					if (picUrl != NULL && !is_error(picUrl))
					{
						keyValues["picUrl"] = json_object_get_string(picUrl);
					}

				}
			}
			else {
				keyValues[key] = json_object_get_string(val);
			}
		}

		if (keyValues.find("resultCode") == keyValues.end())
		{
			keyValues["resultCode"] = "0";
		}

		if (keyValues["resultCode"] != "0" && keyValues.find("failReason") == keyValues.end())
		{
			json = json_object_object_get(root, "return_message");
			if (!is_error(json))
			{
				value = json_object_get_string(json);
				keyValues["failReason"] = value;
			}
		}

		json_object_put(root);
	}
	return 0;
}

std::string ResponseProcess::ConvertUcs2ToAnsi(const std::string& ucs2)
{
	if (ucs2.empty())
	{
		return "";
	}

	// 将含ucs3编码字符串内容转为unicode编码
	std::string utf8;
	for (size_t i = 0; i < ucs2.size();)
	{
		char cur = ucs2[i];
		if (cur == '\\')
		{
			if (i + 1 < ucs2.size())
			{
				char next = ucs2[i + 1];
				if (next == 'u' || next == 'U')
				{
					if (i + 5 < ucs2.size())
					{
						// 将\uffff编码形式的字符串转为utf8二进制内容
						std::string ucs2CharCode = ucs2.substr(i + 2, 4);
						transform(ucs2CharCode.begin(), ucs2CharCode.end(), ucs2CharCode.begin(), ::tolower);
						unsigned short code = (unsigned short)strtoul(ucs2CharCode.c_str(), NULL, 16);

						if (code > 0)
						{
							if (0x0080 > code)
							{
								/* 1 byte UTF-8 Character.*/
								utf8.append(1, (char)code);
							}
							else if (0x0800 > code)
							{
								/*2 bytes UTF-8 Character.*/
								utf8.append(1, 0xc0 | ((char)(code >> 6)));
								utf8.append(1, 0x80 | ((char)(code & 0x3F)));
							}
							else
							{
								/* 3 bytes UTF-8 Character .*/
								utf8.append(1, 0xE0 | ((char)(code >> 12)));
								utf8.append(1, 0x80 | ((char)((code >> 6) & 0x3F)));
								utf8.append(1, 0x80 | ((char)(code & 0x3F)));
							}

							i += 6;
							continue;
						}
					}
				}
			}
		}
		utf8.append(1, cur);
		++i;
	}

	std::string ansi;

	// 将utf8编码转为unicode编码
	int unicodeBytesLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), NULL, 0);
	WCHAR* unicodeBytes = new WCHAR[unicodeBytesLen];
	if (unicodeBytesLen == MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), unicodeBytes, unicodeBytesLen))
	{
		// 将unicode编码转为ansi编码
		int ansiBytesLen = WideCharToMultiByte(CP_ACP, 0, unicodeBytes, unicodeBytesLen, NULL, 0, NULL, NULL);
		char* ansiBytes = new char[ansiBytesLen];
		if (ansiBytesLen == WideCharToMultiByte(CP_ACP, 0, unicodeBytes, unicodeBytesLen, ansiBytes, ansiBytesLen, NULL, NULL))
		{
			ansi.assign(ansiBytes, ansiBytesLen);
		}
		delete[] ansiBytes;
	}
	delete[] unicodeBytes;

	return ansi;
}

void RequestProcess::SetCommonParam(HttpRequest* request, bool isGet)
{
	request->hostName = m_hostName;
	request->port = m_port;
	request->hostName2 = m_hostName2;
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;

	string* strurl;
	if (isGet)
	{
		request->method = "GET";
		strurl = &request->url;
	}
	else
	{
		request->method = "POST";
		strurl = &request->postData;
	}

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	if (m_macId.size() == 0) {
		char szTemp[128] = { 0 };
		int nTempSize = 128;
		CComputerInfo::GetMacAddress(szTemp, nTempSize);
		m_macId = szTemp;
	}

	//计算机名称
	std::wstring wstring_computerName;
	if (GetComputerNameInterface(wstring_computerName))
	{

	}
	std::string string_computer = LocalUnicodeToUtf8(wstring_computerName);
	std::string encodedName = UrlEncoder::encode_go_down(string_computer.c_str());

	std::string extendInfo;
	if (adTraceId.size() > 0)
	{
		extendInfo = ConstructJson(adTraceId);
	}
	std::string extendInfo_encode = UrlEncoder::encode_go_down(extendInfo.c_str());

	char buff[1024];
	if (m_appid == 88 || m_appid == 11 || m_appid == 791000810)
	{
		sprintf(buff, "&authenSource=1&appId=%d&areaId=%d&groupId=%d&appIdSite=%d&locale=%s&productId=%d&frameType=1&endpointOS=1&version=21&customSecurityLevel=%d&deviceId=%s&thirdLoginExtern=%s&macId=%s&epIp=%s&epName=%s&extendInfo=%s&sdoVersion=%s&runTimeId=%s&channelId=%s",
			m_appid, m_areaid, m_groupId, m_appid, m_locale != 2 ? "zh_CN" : "en_US", m_productId, m_customSecurityLevel, m_deviceId.c_str(), LoginClient::getThirdLoginExtern().c_str(), m_macId.c_str(), m_IpId.c_str(), encodedName.c_str(), extendInfo_encode.c_str(), sdoVersion.c_str(), sdoRunTimerId.c_str(), LoginClient::getChannelIdExtern().c_str());
	}
	else
	{
		sprintf(buff, "&authenSource=1&appId=%d&areaId=%d&groupId=%d&appIdSite=%d&locale=%s&productId=%d&frameType=1&endpointOS=1&version=21&customSecurityLevel=%d&deviceId=%s&thirdLoginExtern=%s&macId=%s&epIp=%s&epName=%s&extendInfo=%s&sdoVersion=%s&runTimeId=%s&channelId=%s",
			m_appid, m_areaid, m_groupId, m_appid, m_locale != 2 ? "zh_CN" : "en_US", m_productId, m_customSecurityLevel, m_deviceId.c_str(), LoginClient::getThirdLoginExtern().c_str(), m_macId.c_str(), m_IpId.c_str(), encodedName.c_str(), extendInfo_encode.c_str(), sdoVersion.c_str(), sdoRunTimerId.c_str(), LoginClient::getChannelIdExtern().c_str());
	}
	*strurl += buff;
	if (!m_productVersion.empty())
	{
		*strurl += "&productVersion=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}
	if (m_tag != -1)
	{
		sprintf(buff, "&tag=%d", m_tag);
		*strurl += buff;
	}
}

void RequestProcess::SetCalculateCommonParam(HttpRequest* request, bool isGet)
{
	request->hostName = "apis.tianapi.com";
	request->port = m_port;
	request->hostName2 = "apis.tianapi.com";
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;

	string* strurl;
	if (isGet)
	{
		request->method = "GET";
		strurl = &request->url;
	}
	else
	{
		request->method = "POST";
		strurl = &request->postData;
	}
}

void RequestProcess::SetCommonParam(HttpRequest* request, int appId, bool isGet)
{
	request->hostName = m_hostName;
	request->port = m_port;
	request->hostName2 = m_hostName2;
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;

	string* strurl;
	if (isGet)
	{
		request->method = "GET";
		strurl = &request->url;
	}
	else
	{
		request->method = "POST";
		strurl = &request->postData;
	}

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	if (m_macId.size() == 0) {
		char szTemp[128] = { 0 };
		int nTempSize = 128;
		CComputerInfo::GetMacAddress(szTemp, nTempSize);
		m_macId = szTemp;
	}

	//计算机名称
	std::wstring wstring_computerName;
	if (GetComputerNameInterface(wstring_computerName))
	{

	}
	std::string string_computer = LocalUnicodeToUtf8(wstring_computerName);
	std::string encodedName = UrlEncoder::encode_go_down(string_computer.c_str());

	std::string extendInfo;
	if (adTraceId.size() > 0)
	{
		extendInfo = ConstructJson(adTraceId);
	}
	std::string extendInfo_encode = UrlEncoder::encode_go_down(extendInfo.c_str());

	char buff[1024];
	if (m_appid == 88 || m_appid == 11 || m_appid == 791000810)
	{
		sprintf(buff, "&authenSource=1&appId=%d&areaId=%d&groupId=%d&appIdSite=%d&locale=%s&productId=%d&frameType=1&endpointOS=1&version=21&customSecurityLevel=%d&deviceId=%s&thirdLoginExtern=%s&macId=%s&epIp=%s&epName=%s&extendInfo=%s&sdoVersion=%s&runTimeId=%s&channelId=%s",
			appId, m_areaid, m_groupId, appId, m_locale != 2 ? "zh_CN" : "en_US", m_productId, m_customSecurityLevel, m_deviceId.c_str(), LoginClient::getThirdLoginExtern().c_str(), m_macId.c_str(), m_IpId.c_str(), encodedName.c_str(), extendInfo_encode.c_str(), sdoVersion.c_str(), sdoRunTimerId.c_str(), LoginClient::getChannelIdExtern().c_str());
	}
	else
	{
		sprintf(buff, "&authenSource=1&appId=%d&areaId=%d&groupId=%d&appIdSite=%d&locale=%s&productId=%d&frameType=1&endpointOS=1&version=21&customSecurityLevel=%d&deviceId=%s&thirdLoginExtern=%s&macId=%s&epIp=%s&epName=%s&extendInfo=%s&sdoVersion=%s&runTimeId=%s&channelId=%s",
			appId, m_areaid, m_groupId, appId, m_locale != 2 ? "zh_CN" : "en_US", m_productId, m_customSecurityLevel, m_deviceId.c_str(), LoginClient::getThirdLoginExtern().c_str(), m_macId.c_str(), m_IpId.c_str(), encodedName.c_str(), extendInfo_encode.c_str(), sdoVersion.c_str(), sdoRunTimerId.c_str(), LoginClient::getChannelIdExtern().c_str());
	}
	*strurl += buff;
	if (!m_productVersion.empty())
	{
		*strurl += "&productVersion=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}
	if (m_tag != -1)
	{
		sprintf(buff, "&tag=%d", m_tag);
		*strurl += buff;
	}
}

void RequestProcess::SetCommonParam(HttpRequest* request, const char* scene, bool isGet)
{
	request->hostName = m_hostName;
	request->port = m_port;
	request->hostName2 = m_hostName2;
	request->port2 = m_port2;
	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;

	string* strurl;
	if (isGet)
	{
		request->method = "GET";
		strurl = &request->url;
	}
	else
	{
		request->method = "POST";
		strurl = &request->postData;
	}

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	if (m_macId.size() == 0) {
		char szTemp[128] = { 0 };
		int nTempSize = 128;
		CComputerInfo::GetMacAddress(szTemp, nTempSize);
		m_macId = szTemp;
	}

	//计算机名称
	std::wstring wstring_computerName;
	if (GetComputerNameInterface(wstring_computerName))
	{

	}
	std::string string_computer = LocalUnicodeToUtf8(wstring_computerName);
	std::string encodedName = UrlEncoder::encode_go_down(string_computer.c_str());

	std::string extendInfo;
	if (adTraceId.size() > 0)
	{
		extendInfo = ConstructJson(adTraceId);
	}
	std::string extendInfo_encode = UrlEncoder::encode_go_down(extendInfo.c_str());

	char buff[1024];
	if (m_appid == 88 || m_appid == 11 || m_appid == 791000810)
	{
		sprintf(buff, "&authenSource=1&appId=%d&areaId=%d&groupId=%d&appIdSite=%d&locale=%s&productId=%d&frameType=1&endpointOS=1&version=21&customSecurityLevel=%d&deviceId=%s&thirdLoginExtern=%s&macId=%s&epIp=%s&epName=%s&scene=%s&extendInfo=%s&sdoVersion=%s&runTimeId=%s&channelId=%s",
			m_appid, m_areaid, m_groupId, m_appid, m_locale != 2 ? "zh_CN" : "en_US", m_productId, m_customSecurityLevel, m_deviceId.c_str(), LoginClient::getThirdLoginExtern().c_str(), m_macId.c_str(), m_IpId.c_str(), encodedName.c_str(), scene, extendInfo_encode.c_str(), sdoVersion.c_str(), sdoRunTimerId.c_str(), LoginClient::getChannelIdExtern().c_str());
	}
	else
	{
		sprintf(buff, "&authenSource=1&appId=%d&areaId=%d&groupId=%d&appIdSite=%d&locale=%s&productId=%d&frameType=1&endpointOS=1&version=21&customSecurityLevel=%d&deviceId=%s&thirdLoginExtern=%s&macId=%s&epIp=%s&epName=%s&scene=%s&extendInfo=%s&sdoVersion=%s&runTimeId=%s&channelId=%s",
			m_appid, m_areaid, m_groupId, m_appid, m_locale != 2 ? "zh_CN" : "en_US", m_productId, m_customSecurityLevel, m_deviceId.c_str(), LoginClient::getThirdLoginExtern().c_str(), m_macId.c_str(), m_IpId.c_str(), encodedName.c_str(), scene, extendInfo_encode.c_str(), sdoVersion.c_str(), sdoRunTimerId.c_str(), LoginClient::getChannelIdExtern().c_str());
	}
	*strurl += buff;
	if (!m_productVersion.empty())
	{
		*strurl += "&productVersion=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}
	if (m_tag != -1)
	{
		sprintf(buff, "&tag=%d", m_tag);
		*strurl += buff;
	}
}

void RequestProcess::SetAdvCommonParam(HttpRequest* request, bool isGet)
{
	request->hostName = "adi.u.sdo.com";
	request->port = m_port;
	request->hostName2 = "adi.u.sdo.com";
	request->port2 = m_port2;
	request->timeout = 1000;
	request->timeout2 = 1000;

	string* strurl;
	if (isGet)
	{
		request->method = "GET";
		strurl = &request->url;
	}
	else
	{
		request->method = "POST";
		strurl = &request->url;  // POST 请求改为修改 URL，而非 postData
	}

	request->postData = request->postData + std::string("&uniqueId=") + uniqueId +
		"&channel=" + "windows";
}

void RequestProcess::SetGetFaceUrlCommonParam(HttpRequest* request, bool isGet)
{
	request->hostName = "selfservice.sdo.com";
	request->port = m_port;
	request->hostName2 = "selfservice.sdo.com";
	request->port2 = m_port2;
	if (m_appid == 791000814 || m_appid == 791000829 || m_appid == 100001900)
	{
		request->timeout = 5000;
		request->timeout2 = 5000;
	}
	else
	{
		request->timeout = m_timeout;
		request->timeout2 = m_timeout2;
	}

	string* strurl;
	if (isGet)
	{
		request->method = "GET";
		strurl = &request->url;
	}
	else
	{
		request->method = "POST";
		strurl = &request->postData;
	}

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	if (m_macId.size() == 0) {
		char szTemp[128] = { 0 };
		int nTempSize = 128;
		CComputerInfo::GetMacAddress(szTemp, nTempSize);
		m_macId = szTemp;
	}

	//计算机名称
	std::wstring wstring_computerName;
	if (GetComputerNameInterface(wstring_computerName))
	{

	}
	std::string string_computer = LocalUnicodeToUtf8(wstring_computerName);
	std::string encodedName = UrlEncoder::encode_go_down(string_computer.c_str());

	std::string extendInfo;
	if (adTraceId.size() > 0)
	{
		extendInfo = ConstructJson(adTraceId);
	}
	std::string extendInfo_encode = UrlEncoder::encode_go_down(extendInfo.c_str());

	char buff[1024];
	if (m_appid == 88 || m_appid == 11 || m_appid == 791000810)
	{
		sprintf(buff, "&authenSource=1&appId=%d&areaId=%d&appIdSite=%d&locale=%s&productId=%d&frameType=1&endpointOS=1&version=21&customSecurityLevel=%d&deviceId=%s&thirdLoginExtern=%s&macId=%s&epIp=%s&epName=%s&extendInfo=%s&sdoVersion=%s&runTimeId=%s&channelId=%s",
			m_appid, m_areaid, m_appid, m_locale != 2 ? "zh_CN" : "en_US", m_productId, m_customSecurityLevel, m_deviceId.c_str(), LoginClient::getThirdLoginExtern().c_str(), m_macId.c_str(), m_IpId.c_str(), encodedName.c_str(), extendInfo_encode.c_str(), sdoVersion.c_str(), sdoRunTimerId.c_str(), LoginClient::getChannelIdExtern().c_str());
	}
	else
	{
		sprintf(buff, "&authenSource=1&appId=%d&areaId=%d&appIdSite=%d&locale=%s&productId=%d&frameType=1&endpointOS=1&version=21&customSecurityLevel=%d&deviceId=%s&thirdLoginExtern=%s&macId=%s&epIp=%s&epName=%s&extendInfo=%s&sdoVersion=%s&runTimeId=%s&channelId=%s",
			m_appid, m_areaid, m_appid, m_locale != 2 ? "zh_CN" : "en_US", m_productId, m_customSecurityLevel, m_deviceId.c_str(), LoginClient::getThirdLoginExtern().c_str(), m_macId.c_str(), m_IpId.c_str(), encodedName.c_str(), extendInfo_encode.c_str(), sdoVersion.c_str(), sdoRunTimerId.c_str(), LoginClient::getChannelIdExtern().c_str());
	}
	*strurl += buff;
	if (!m_productVersion.empty())
	{
		*strurl += "&productVersion=" + UrlEncoder::encode_go_down(m_productVersion.c_str());
	}
	if (m_tag != -1)
	{
		sprintf(buff, "&tag=%d", m_tag);
		*strurl += buff;
	}
}

void RequestProcess::SetCommonParam(
	HttpRequest* request,
	const char* gunionAppId,
	const std::string& gunionToken,
	const std::string& gunionSignature,
	bool isGet)
{
	if (!isGet)
	{
		request->hostName = m_hostName7;
		request->port = m_port;
		request->hostName2 = m_hostName8;
		request->port2 = m_port2;
	}
	else
	{
		request->hostName = "utility.sdoprofile.com";
		request->port = m_port;
		request->hostName2 = "utility.sdoprofile.com";
		request->port2 = m_port2;
	}

	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;

	string* strurl;
	if (isGet)
	{
		request->method = "GET";
		strurl = &request->url;
	}
	else
	{
		request->method = "POST";
		strurl = &request->postData;
	}

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	std::string string_appid(gunionAppId);

	char strBuf[32];
	sprintf(strBuf, "%d", m_areaid);
	std::string areaidStr(strBuf);

	if (string_appid.size() > 0)
	{
		request->AddHeader("X-APP-ID", string_appid);
	}
	request->AddHeader("X-PLATFORM", "3");
	request->AddHeader("X-CHANNEL", "windows");
	if (!gunionToken.empty())
	{
		request->AddHeader("X-TOKEN", gunionToken);
	}
	if (areaidStr.size() > 0)
	{
		request->AddHeader("X-AREA", areaidStr);
	}
	if (m_deviceId.size() > 0)
	{
		request->AddHeader("X-DEVICEID", m_deviceId);
	}
	else
	{
		request->AddHeader("X-DEVICEID", GetClientSign2());
	}
	if (adTraceId.size() > 0)
	{
		request->AddHeader("X-AD-TRACEID", adTraceId);
	}
	if (!gunionSignature.empty())
	{
		request->AddHeader("X-SIGNATURE", gunionSignature);
	}
	if (m_productVersion.size() > 0)
	{
		request->AddHeader("X-SDK-VERSION", m_productVersion.c_str());
	}
	std::string externVal = LoginClient::getChannelIdExtern();
	if (!externVal.empty() && externVal != "0")
	{
		request->AddHeader("X-APPLICATION-CHANNEL", LoginClient::getChannelIdExtern());
	}
	if (sdoRunTimerId.size() > 0)
	{
		request->AddHeader("X-RUNNING-ID", sdoRunTimerId);
	}
}

void RequestProcess::SetCheckPayEntraceCommonParam(
	HttpRequest* request,
	const char* gunionAppId,
	int iSSandboxAccount,
	const std::string& gunionToken,
	const std::string& gunionSignature,
	bool isGet)
{
	if (iSSandboxAccount == 0)
	{
		request->hostName = m_hostName7;
		request->port = m_port;
		request->hostName2 = m_hostName8;
		request->port2 = m_port2;
	}
	else
	{
		request->hostName = "mgame-gray.sdo.com";
		request->port = m_port;
		request->hostName2 = "mgame-gray.sdo.com";
		request->port2 = m_port2;
	}

	request->timeout = m_timeout;
	request->timeout2 = m_timeout2;

	string* strurl;
	if (isGet)
	{
		request->method = "GET";
		strurl = &request->url;
	}
	else
	{
		request->method = "POST";
		strurl = &request->postData;
	}

	if (m_deviceId.size() == 0)
	{
		m_deviceId = GetClientSign2();
	}

	std::string string_appid(gunionAppId);

	char strBuf[32];
	sprintf(strBuf, "%d", m_areaid);
	std::string areaidStr(strBuf);

	if (string_appid.size() > 0)
	{
		request->AddHeader("X-APP-ID", string_appid);
	}
	request->AddHeader("X-PLATFORM", "3");
	request->AddHeader("X-CHANNEL", "windows");
	if (!gunionToken.empty())
	{
		request->AddHeader("X-TOKEN", gunionToken);
	}
	if (areaidStr.size() > 0)
	{
		request->AddHeader("X-AREA", areaidStr);
	}
	if (m_deviceId.size() > 0)
	{
		request->AddHeader("X-DEVICEID", m_deviceId);
	}
	else
	{
		request->AddHeader("X-DEVICEID", GetClientSign2());
	}
	if (adTraceId.size() > 0)
	{
		request->AddHeader("X-AD-TRACEID", adTraceId);
	}
	if (!gunionSignature.empty())
	{
		request->AddHeader("X-SIGNATURE", gunionSignature);
	}
	if (m_productVersion.size() > 0)
	{
		request->AddHeader("X-SDK-VERSION", m_productVersion.c_str());
	}
	std::string externVal = LoginClient::getChannelIdExtern();
	if (!externVal.empty() && externVal != "0")
	{
		request->AddHeader("X-APPLICATION-CHANNEL", LoginClient::getChannelIdExtern());
	}
	if (sdoRunTimerId.size() > 0)
	{
		request->AddHeader("X-RUNNING-ID", sdoRunTimerId);
	}
}


