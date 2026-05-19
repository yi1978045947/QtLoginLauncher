/**
 * CurlHttpThread.cpp
 * --------------------------------------------------------------------
 * 本文件是对项目中 HttpThread（WinInet 版）的完全镜像替代。
 * 唯一区别是内部 HTTP 客户端从 WinInet 改为 CurlHttpClient。
 *
 * 功能：
 *   保持原 HttpThread 的行为和结构 100% 一致（逐行迁移）
 *   使用 curl 完成 HTTP 请求 → 不再受系统 TLS 影响
 *   支持后台线程执行网络请求
 *   支持 Cancel
 *   支持多域名轮询（host1/host2 → host3/host4 → host5/host6）
 *   完全保留 publicKey 的获取流程（GetDynamicKey 依赖）
 *   完全复刻原来的 goto HOST_BACKUP 状态机
 *   保证 LoginClient 端不需要做任何修改
 *
 * 调用架构：
 *
 *   LoginClient
 *        ↓
 *   CurlHttpThread.ProcessRequest()
 *        ↓ (signal m_event)
 *   后台线程 ProcessThread()
 *        ↓
 *   CurlHttpClient.SendHttpRequest()
 *        ↓
 *   将结果回调给 LoginClient（ProcessResponse / ClearPushMessageVerifyCheckCodeStatus）
 *
 * ★ 注意：为了保持旧行为，所有 static 变量都必须保留。★
 */

#include "StdAfx.h"
#include "CurlHttpThread.h"
#include "LoginClient.h"
#include "MsgProcess.h"

#include <process.h>
#include <iostream>

 ///////////////////////////////////////////////////////////////////////////////
 // 构造 & 析构
 ///////////////////////////////////////////////////////////////////////////////

CurlHttpThread::CurlHttpThread(LoginClient* loginClient)
	: m_loginClient(loginClient)
	, m_state(STATE_IDLE)
	, m_running(true)
	, m_request(NULL)
	, m_proxyEnable(true)
	, m_inUserData(NULL)
	, m_outUserData(NULL)
	, m_hostPort3(0)
	, m_hostPort4(0)
	, m_hostPort5(0)
	, m_hostPort6(0)
{
	// 自动重置事件 —— 每次处理完自动变回未触发状态
	m_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// 创建后台线程
	m_thread = (HANDLE)_beginthreadex(
		NULL,                     // 默认安全属性
		0,                        // 默认栈大小
		&CurlHttpThread::ThreadEntry,
		this,
		0,
		NULL);
}

CurlHttpThread::~CurlHttpThread()
{
	m_running = false;

	// 唤醒线程，使其退出循环
	::SetEvent(m_event);

	// 等待线程自然退出
	::WaitForSingleObject(m_thread, INFINITE);

	::CloseHandle(m_thread);
	::CloseHandle(m_event);

	delete m_request;
}

///////////////////////////////////////////////////////////////////////////////
// 状态控制接口
///////////////////////////////////////////////////////////////////////////////

int CurlHttpThread::GetState()
{
	return m_state;
}

void CurlHttpThread::ProxyEnable(int enable)
{
	m_proxyEnable = (enable != 0);
}

void CurlHttpThread::SetUserData(const void* userData)
{
	m_inUserData = (void*)userData;
}

void* CurlHttpThread::GetUserData()
{
	return m_outUserData;
}

///////////////////////////////////////////////////////////////////////////////
// 设置 HTTPS 证书校验模式
// ---------------------------------------------------------------------------
// enable = true  → 启用证书校验（生产环境的正常安全策略）
// enable = false → 禁用证书校验（调试 / 抓包场景，例如 Fiddler、HttpAnalyzer）
//
// 说明：
//  - CurlHttpThread 不直接处理证书逻辑，而是转发给内部的 CurlHttpClient。
//  - CurlHttpClient 会在 SendHttpRequest() 中根据此标志决定是否执行
//    CURLOPT_SSL_VERIFYPEER / CURLOPT_SSL_VERIFYHOST。
//  - 默认保持安全模式（true），业务可按需切换。
// ---------------------------------------------------------------------------
// 用途举例：
//   httpThread->SetVerifyCert(false);  // 调试抓包
//   httpThread->SetVerifyCert(true);   // 恢复正常校验
///////////////////////////////////////////////////////////////////////////////
void CurlHttpThread::SetVerifyCert(bool enable)
{
	// 转发到底层 HTTP 客户端
	m_httpClient.SetVerifyCert(enable);
}

/**
 * IsCDomainRequest
 * ------------------------------------------------------------
 * 当前仅判断 mgame.sdo.com 域名组接口 / 广告初始化接口。
 * 若未来新增接口，只需在此处增加 case，
 * 不需要修改线程核心逻辑。
 */
bool CurlHttpThread::IsCDomainRequest(int requestCode)
{
	switch (requestCode)
	{
		// ----------------------------
		// mgame.sdo.com 组：Gunion / WeGame 独立域
		// ----------------------------

	case REQ_GunionWegameInit:
	case REQ_GunionWegameLogin:
	case REQ_GunionWegameSmsSend:
	case REQ_GunionWegamePicSmsSend:
	case REQ_GunionWegameThirdAccountBindPhone:
	case REQ_GunionWegameFillRealinfo:
	case REQ_GunionWegameGetTicket:
	case REQ_GunionWegameGetPayTicket:
	case REQ_GunionGetPublicKey:
	case REQ_GunionHandshake:
	case REQ_GunionSmsSend:
	case REQ_GunionPicSmsSend:
	case REQ_GunionSmsLogin:
	case REQ_GunionSpecialSmsSend:
	case REQ_GunionSpecialPicSmsSend:
	case REQ_GunionSpecialConfirmSmsSend:
	case REQ_GunionSpecialSmsLogin:
	case REQ_GunionPwdLogin:
	case REQ_GunionPicPwdLogin:
	case REQ_GunionSpecialPwdLogin:
	case REQ_GunionSpecialPicPwdLogin:
	case REQ_GunionThirdLogin:
	case REQ_GunionAutoLogin:
	case REQ_GunionLogout:
	case REQ_GunionRealName:
	case REQ_GunionGetLoginArea:
	case REQ_GunionSetLoginArea:
	case REQ_GunionPayEntrance:
	case REQ_GunionPayOrderStatus:
	case REQ_GunionActivation:
	case REQ_GunionSetTutorLogin:
	case REQ_GunionWegameChannelLogin:
	case REQ_GunionGetPrivateUrl:
	case REQ_GunionGetAgreementContent:
	case REQ_CheckInitAdv:
	case REQ_GunionLoginGetPublicKey:
	case REQ_GunionLoginHandshake:
	case REQ_UserPrivacyConfig:
	case REQ_CreateCQ4Order:
	case REQ_CreateCQ4Query:
	case REQ_CreateSteamChannelOrder:
	case REQ_CreateQQGameOrder:
	case REQ_CreateLxOrder:
	case REQ_FaceCodeResult:
	case REQ_FaceSendAction:
	case REQ_SteamPayResult:
	case REQ_SteamChannelPayResult:
	case REQ_CreateWeGameOrder:
	case REQ_NewVersionUserPrivacyConfig:
		return true;

	default:
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// 提交请求
///////////////////////////////////////////////////////////////////////////////

int CurlHttpThread::ProcessRequest(HttpRequest* request)
{
	if (m_state == STATE_BUSY)
		return -1;

	m_state = STATE_BUSY;

	// 将用户数据附加到 Request 上
	request->userData = m_inUserData;

	m_request = request;

	// 激活后台线程
	::SetEvent(m_event);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Cancel
///////////////////////////////////////////////////////////////////////////////

int CurlHttpThread::Cancel()
{
	// 保持原有写法：STATE_BUSY 才能 Cancel
	if (m_state == STATE_BUSY)
		return m_httpClient.Cancel();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// 线程入口
///////////////////////////////////////////////////////////////////////////////

unsigned __stdcall CurlHttpThread::ThreadEntry(void* param)
{
	return ((CurlHttpThread*)param)->ProcessThread();
}

///////////////////////////////////////////////////////////////////////////////
// 核心线程逻辑（完整复刻旧 HttpThread）
///////////////////////////////////////////////////////////////////////////////

//unsigned CurlHttpThread::ProcessThread()
//{
//	while (m_running)
//	{
//		::WaitForSingleObject(m_event, INFINITE);
//
//		if (!m_running)
//			break;
//
//		HttpRequest* request = m_request;
//		m_request = NULL;
//
//		if (request)
//		{
//			string response;
//			vector<string> vecCookies;
//
//			int ret = 0;         // 当前 HTTP 结果
//			bool isUrl2 = false; // 是否使用 URL2（备用 URL）
//
//			//----------------------------------------------------------
//			// ★★★ 以下四个 static 变量决定整个登录流程的状态机 ★★★
//			//----------------------------------------------------------
//
//			static bool  isUrlFailed = false; // 上次 host1 是否失败过
//			static string publicKey = "";    // 动态 key 所需公钥（缓存一次）
//			static bool  isHostBackup3or4 = false; // 是否已经切换到 host3/host4
//			static bool g_hasSwitchedToBackupGroup = false;  // 全局标记：是否已经切换过备用域名
//			bool switchedToBackupGroup = false; // 局部变量：是否已经切过 host	
//
//			// ============================================================
//			// 若已切备用域，则强制这主域名是n.cas.sdo的固定接口也使用备用域
//			// ============================================================
//			if (g_hasSwitchedToBackupGroup)
//			{
//				if (request->requestCode == REQ_GetClientConfig || request->requestCode == REQ_NewVersionFastLogin
//					|| request->requestCode == REQ_NewVersionGoDownConfigInit || request->requestCode == REQ_NewVersionGoDownSendSmsCheckCode
//					|| request->requestCode == REQ_NewVersionGoDownconfirmSendSms || request->requestCode == REQ_NewVersionGoDownconfirmLogin
//					|| request->requestCode == REQ_NewVersionGetTicket || request->requestCode == REQ_FastLoginActiveDeleteAccount
//					|| request->requestCode == REQ_GuardDianSendSms || request->requestCode == REQ_GuardDianSendSmsCheckCode
//					|| request->requestCode == REQ_GuardDianConfrimSendSmsResult)
//				{
//					request->hostName = m_hostName3; // 备用组主域
//					request->port = m_hostPort3;
//					request->hostName2 = m_hostName4; // 备用组备用域
//					request->port2 = m_hostPort4;
//				}
//			}
//
//			//--------------------------------------------------------------
//			// ★★★ HOST_BACKUP 标签（保持旧行为，不可删除，不可改结构）★★★
//			//--------------------------------------------------------------
//		HOST_BACKUP:
//
//			int count = 0; // 记录是否尝试过 host1（主域名）
//
//			/*******************************************************************
//			 * 第一阶段：尝试使用主域名 host1（request->hostName）
//			 * -----------------------------------------------------------------
//			 * 条件：isUrlFailed == false（即之前没有失败）
//			 *******************************************************************/
//			if (!isUrlFailed)
//			{
//				ret = m_httpClient.SendHttpRequest(
//					request->hostName,
//					request->port,
//					request->url,
//					request->method,
//					request->postData,
//					request->timeout,
//					response,
//					vecCookies,
//					m_proxyEnable,
//					request->headers);
//
//				count++; // 已尝试 host1
//
//				// 标记 host1 是否失败
//				isUrlFailed = (ret != 0);
//			}
//
//			/*******************************************************************
//			 * 清除短信校验状态
//			 * -----------------------------------------------------------------
//			 * 特殊逻辑：
//			 *   只有短信验证（REQ_SendPushMessageVerifyCheckCode）不清空
//			 *******************************************************************/
//			if (request->requestCode != REQ_SendPushMessageVerifyCheckCode && m_loginClient)
//			{
//				m_loginClient->ClearPushMessageVerifyCheckCodeStatus();
//			}
//
//			/*******************************************************************
//			 * 特殊逻辑：首次接口调用失败并且接口获取动态 key（REQ_GetDynamicKey）
//			 * -----------------------------------------------------------------
//			 * 必须先拿到 publicKey，否则动态 key 无法生成
//			 *******************************************************************/
//			if (isUrlFailed &&
//				request->requestCode == REQ_GetDynamicKey &&
//				publicKey.length() == 0)
//			{
//				//----------------------------------------------------------
//				// 先使用 hostName2 请求 PublicKey（由业务层提供第二域名）
//				//----------------------------------------------------------
//				int r = GetPublicKey(publicKey, response, vecCookies, m_proxyEnable);
//
//				if (r != 0 || publicKey.length() == 0)
//				{
//					//------------------------------------------------------
//					// publicKey 获取失败 → 尝试 host3/host4 备援机制
//					//------------------------------------------------------
//					if (!isHostBackup3or4)
//					{
//						// 切换域名到 host3 & host4
//						request->hostName = m_hostName3;
//						request->port = m_hostPort3;
//						request->hostName2 = m_hostName4;
//						request->port2 = m_hostPort4;
//
//						isUrlFailed = false;
//						ret = 0;
//						publicKey = "";
//						response = "";
//						isHostBackup3or4 = true;
//						g_hasSwitchedToBackupGroup = true;
//
//						// 通知 LoginClient 更新 host3/4 信息
//						m_loginClient->SetHost3AndHost4(
//							m_hostName3, m_hostPort3,
//							m_hostName4, m_hostPort4);
//
//						// ★ 再尝试一次，从 HOST_BACKUP 开始
//						goto HOST_BACKUP;
//					}
//
//					// 已备援且 still 失败 → 返回失败
//					m_state = STATE_IDLE;
//					m_outUserData = request->userData;
//
//					m_loginClient->ProcessResponse(r, request->requestCode,
//						response, vecCookies, isUrl2);
//
//					delete request;
//					continue;
//				}
//				else
//				{
//					//------------------------------------------------------
//					// publicKey 成功 → 构造新的 GetDynamicKey 请求
//					//------------------------------------------------------
//
//					m_outUserData = request->userData;
//					delete request;
//
//					// 清空旧 response & cookies
//					response.clear();
//					vecCookies.clear();
//
//					// 生成新的 REQ_GetDynamicKey 请求（使用 publicKey）
//					request = m_loginClient->GetDynamicKeyRequest(publicKey);
//
//					request->userData = m_outUserData;
//				}
//			}
//
//			// ================================
//			// 其他接口失败时也切换备用域名 host3/host4
//			// ================================
//			if (ret != 0 && request->requestCode != REQ_GetDynamicKey)
//			{
//				if (!switchedToBackupGroup && !g_hasSwitchedToBackupGroup)
//				{
//					// 初始化host3和host4状态
//					request->hostName = m_hostName3;
//					request->port = m_hostPort3;
//					request->hostName2 = m_hostName4;
//					request->port2 = m_hostPort4;
//					isUrlFailed = false;
//					ret = 0;
//					response = "";
//					m_loginClient->SetHost3AndHost4(m_hostName3, m_hostPort3, m_hostName4, m_hostPort4);
//					switchedToBackupGroup = true;
//					g_hasSwitchedToBackupGroup = true; // 全局标记
//					// 保持与原逻辑一致：
//					// host3 -> host4 最多访问两次，不会循环多次
//
//					// 直接切到备用组
//					goto HOST_BACKUP;
//				}
//			}
//
//			/*******************************************************************
//			 * 第二阶段：如果 host1 没尝试过（count == 0），那就试 host2 / url2
//			 *******************************************************************/
//			if (count == 0)
//			{
//				string url = request->url2.length() > 0 ? request->url2 : request->url;
//
//				ret = m_httpClient.SendHttpRequest(
//					request->hostName2,
//					request->port2,
//					url,
//					request->method,
//					request->postData,
//					request->timeout2,
//					response,
//					vecCookies,
//					m_proxyEnable,
//					request->headers);
//
//				isUrl2 = true;
//			}
//
//			/*******************************************************************
//			 * 第三阶段：如果前面都失败，SPECIAL fallback：再试一次 host2
//			 *******************************************************************/
//			if (ret != 0 && !request->hostName2.empty())
//			{
//				string url = request->url2.length() > 0 ? request->url2 : request->url;
//
//				ret = m_httpClient.SendHttpRequest(
//					request->hostName2,
//					request->port2,
//					url,
//					request->method,
//					request->postData,
//					request->timeout2,
//					response,
//					vecCookies,
//					m_proxyEnable,
//					request->headers);
//
//				isUrl2 = true;
//			}
//
//			/*******************************************************************
//			 * 全流程结束 → 回调 LoginClient 处理结果
//			 *******************************************************************/
//			m_state = STATE_IDLE;
//			m_outUserData = request->userData;
//
//			m_loginClient->ProcessResponse(ret, request->requestCode,
//				response, vecCookies, isUrl2);
//
//			delete request;
//		}
//	}
//
//	return 0;
//}

unsigned CurlHttpThread::ProcessThread()
{
	while (m_running)
	{
		::WaitForSingleObject(m_event, INFINITE);

		if (!m_running)
			break;

		HttpRequest* request = m_request;
		m_request = NULL;

		if (request)
		{
			string response;
			vector<string> vecCookies;

			int ret = 0;         // 当前 HTTP 结果
			bool isUrl2 = false; // 是否使用 URL2（备用 URL）

			//----------------------------------------------------------
			// 以下四个 static 变量决定整个登录流程的状态机
			//----------------------------------------------------------

			static bool  isUrlFailed = false; // 上次 host1 是否失败过
			static string publicKey = "";    // 动态 key 所需公钥（缓存一次）
			static bool  isHostBackup3or4 = false; // 是否已经切换到 host3/host4
			static bool g_hasSwitchedToBackupGroup = false;  // 全局标记：是否已经切换过备用域名
			bool switchedToBackupGroup = false; // 局部变量：是否已经切过 host	

			// 根据 requestCode 判断是否属于 mgame.sdo.com 域名组
			bool isCDomain = IsCDomainRequest(request->requestCode);

			// ============================================================
			// C 组：mgame.sdo.com / Gunion 完全独立流程
			// ============================================================
			if (isCDomain)
			{
				// 第一次：host5
				ret = m_httpClient.SendHttpRequest(
					request->hostName,
					request->port,
					request->url,
					request->method,
					request->postData,
					request->timeout,
					response,
					vecCookies,
					m_proxyEnable,
					request->headers,
					request->requestCode);

				// 第二次：host6
				if (ret != 0 && !request->hostName2.empty())
				{
					string url = request->url2.length() > 0 ? request->url2 : request->url;

					ret = m_httpClient.SendHttpRequest(
						request->hostName2,
						request->port2,
						url,
						request->method,
						request->postData,
						request->timeout2,
						response,
						vecCookies,
						m_proxyEnable,
						request->headers,
						request->requestCode);

					isUrl2 = true;
				}

				// 回调
				m_state = STATE_IDLE;
				m_outUserData = request->userData;

				m_loginClient->ProcessResponse(
					ret,
					request->requestCode,
					response,
					vecCookies,
					isUrl2);

				delete request;
				continue; // mgame.sdo.com 组流程到此结束，绝不进入 cas.sdo.com 组状态机
			}

			// ============================================================
			// 若已切备用域，则强制这主域名是n.cas.sdo的固定接口也使用备用域
			// ============================================================
			if (g_hasSwitchedToBackupGroup)
			{
				if (request->requestCode == REQ_GetClientConfig || request->requestCode == REQ_NewVersionFastLogin
					|| request->requestCode == REQ_NewVersionGoDownConfigInit || request->requestCode == REQ_NewVersionGoDownSendSmsCheckCode
					|| request->requestCode == REQ_NewVersionGoDownconfirmSendSms || request->requestCode == REQ_NewVersionGoDownconfirmLogin
					|| request->requestCode == REQ_NewVersionGetTicket || request->requestCode == REQ_FastLoginActiveDeleteAccount
					|| request->requestCode == REQ_GuardDianSendSms || request->requestCode == REQ_GuardDianSendSmsCheckCode
					|| request->requestCode == REQ_GuardDianConfrimSendSmsResult || request->requestCode == REQ_GetActiveCodeLoginResult)
				{
					request->hostName = m_hostName3; // 备用组主域
					request->port = m_hostPort3;
					request->hostName2 = m_hostName4; // 备用组备用域
					request->port2 = m_hostPort4;
				}
			}

			//--------------------------------------------------------------
			// HOST_BACKUP 标签（保持旧行为，不可删除，不可改结构）
			//--------------------------------------------------------------
		HOST_BACKUP:

			int count = 0; // 记录是否尝试过 host1（主域名）

			/*******************************************************************
			 * 第一阶段：尝试使用主域名 host1（request->hostName）
			 * -----------------------------------------------------------------
			 * 条件：isUrlFailed == false（即之前没有失败）
			 *******************************************************************/
			if (!isUrlFailed)
			{
				ret = m_httpClient.SendHttpRequest(
					request->hostName,
					request->port,
					request->url,
					request->method,
					request->postData,
					request->timeout,
					response,
					vecCookies,
					m_proxyEnable,
					request->headers,
					request->requestCode);

				count++; // 已尝试 host1

				// 标记 host1 是否失败
				isUrlFailed = (ret != 0);
			}

			/*******************************************************************
			 * 清除短信校验状态
			 * -----------------------------------------------------------------
			 * 特殊逻辑：
			 *   只有短信验证（REQ_SendPushMessageVerifyCheckCode）不清空
			 *******************************************************************/
			if (request->requestCode != REQ_SendPushMessageVerifyCheckCode && m_loginClient)
			{
				m_loginClient->ClearPushMessageVerifyCheckCodeStatus();
			}

			/*******************************************************************
			 * 特殊逻辑：首次接口调用失败并且接口获取动态 key（REQ_GetDynamicKey）
			 * -----------------------------------------------------------------
			 * 必须先拿到 publicKey，否则动态 key 无法生成 （仅 cas.sdo.com 组参与）
			 *******************************************************************/
			if (isUrlFailed &&
				request->requestCode == REQ_GetDynamicKey &&
				publicKey.length() == 0)
			{
				//----------------------------------------------------------
				// 先使用 hostName2 请求 PublicKey（由业务层提供第二域名）
				//----------------------------------------------------------
				int r = GetPublicKey(publicKey, response, vecCookies, m_proxyEnable);

				if (r != 0 || publicKey.length() == 0)
				{
					//------------------------------------------------------
					// publicKey 获取失败 → 尝试 host3/host4 备援机制
					//------------------------------------------------------
					if (!isHostBackup3or4)
					{
						// 切换域名到 host3 & host4
						request->hostName = m_hostName3;
						request->port = m_hostPort3;
						request->hostName2 = m_hostName4;
						request->port2 = m_hostPort4;

						isUrlFailed = false;
						ret = 0;
						publicKey = "";
						response = "";
						isHostBackup3or4 = true;
						g_hasSwitchedToBackupGroup = true;

						// 通知 LoginClient 更新 host3/4 信息
						m_loginClient->SetHost3AndHost4(
							m_hostName3, m_hostPort3,
							m_hostName4, m_hostPort4);

						// 再尝试一次，从 HOST_BACKUP 开始
						goto HOST_BACKUP;
					}

					// 已备援且 still 失败 → 返回失败
					m_state = STATE_IDLE;
					m_outUserData = request->userData;

					m_loginClient->ProcessResponse(r, request->requestCode,
						response, vecCookies, isUrl2);

					delete request;
					continue;
				}
				else
				{
					//------------------------------------------------------
					// publicKey 成功 → 构造新的 GetDynamicKey 请求
					//------------------------------------------------------

					m_outUserData = request->userData;
					delete request;

					// 清空旧 response & cookies
					response.clear();
					vecCookies.clear();

					// 生成新的 REQ_GetDynamicKey 请求（使用 publicKey）
					request = m_loginClient->GetDynamicKeyRequest(publicKey);

					request->userData = m_outUserData;
				}
			}

			// ================================
			// 其他接口失败时也切换备用域名 host3/host4 (仅 cas.sdo.com 组）
			// ================================
			if (ret != 0 && request->requestCode != REQ_GetDynamicKey)
			{
				if (!switchedToBackupGroup && !g_hasSwitchedToBackupGroup)
				{
					// 初始化host3和host4状态
					request->hostName = m_hostName3;
					request->port = m_hostPort3;
					request->hostName2 = m_hostName4;
					request->port2 = m_hostPort4;
					isUrlFailed = false;
					ret = 0;
					response = "";
					m_loginClient->SetHost3AndHost4(m_hostName3, m_hostPort3, m_hostName4, m_hostPort4);
					switchedToBackupGroup = true;
					g_hasSwitchedToBackupGroup = true; // 全局标记
					// 保持与原逻辑一致：
					// host3 -> host4 最多访问两次，不会循环多次

					// 直接切到备用组
					goto HOST_BACKUP;
				}
			}

			/*******************************************************************
			 * 第二阶段：如果 host1 没尝试过（count == 0），那就试 host2 / url2
			 *******************************************************************/
			if (count == 0)
			{
				string url = request->url2.length() > 0 ? request->url2 : request->url;

				ret = m_httpClient.SendHttpRequest(
					request->hostName2,
					request->port2,
					url,
					request->method,
					request->postData,
					request->timeout2,
					response,
					vecCookies,
					m_proxyEnable,
					request->headers,
					request->requestCode);

				isUrl2 = true;
			}

			/*******************************************************************
			 * 第三阶段：如果前面都失败，SPECIAL fallback：再试一次 host2
			 *******************************************************************/
			if (ret != 0 && !request->hostName2.empty())
			{
				string url = request->url2.length() > 0 ? request->url2 : request->url;

				ret = m_httpClient.SendHttpRequest(
					request->hostName2,
					request->port2,
					url,
					request->method,
					request->postData,
					request->timeout2,
					response,
					vecCookies,
					m_proxyEnable,
					request->headers,
					request->requestCode);

				isUrl2 = true;
			}

			/*******************************************************************
			 * 全流程结束 → 回调 LoginClient 处理结果
			 *******************************************************************/
			m_state = STATE_IDLE;
			m_outUserData = request->userData;

			m_loginClient->ProcessResponse(ret, request->requestCode,
				response, vecCookies, isUrl2);

			delete request;
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// 公钥获取流程（完全按旧逻辑编写）
///////////////////////////////////////////////////////////////////////////////

int CurlHttpThread::GetPublicKey(string& publicKey,
	string& response,
	vector<string>& vecCookies,
	bool proxyEnable)
{
	// GetPublicKeyRequest 由 LoginClient 构造
	HttpRequest* publicKeyReq = m_loginClient->GetPublicKeyRequest();

	// 第一次尝试
	int ret = m_httpClient.SendHttpRequest(
		publicKeyReq->hostName2,
		publicKeyReq->port2,
		publicKeyReq->url2,
		publicKeyReq->method,
		publicKeyReq->postData,
		publicKeyReq->timeout2,
		response,
		vecCookies,
		proxyEnable);

	// 第二次尝试（完全复刻旧逻辑）
	if (ret != 0)
	{
		ret = m_httpClient.SendHttpRequest(
			publicKeyReq->hostName2,
			publicKeyReq->port2,
			publicKeyReq->url2,
			publicKeyReq->method,
			publicKeyReq->postData,
			publicKeyReq->timeout2,
			response,
			vecCookies,
			proxyEnable);
	}

	// 调用 ResponseProcess 解析 PublicKey
	if (ret == 0)
	{
		map<string, string> keyValues;

		ResponseProcess::Process(m_loginClient, ret, REQ_GetPublicKey, response, vecCookies, keyValues);

		m_loginClient->ParesePublicKey(ret, keyValues, publicKey);
	}

	return ret;
}
