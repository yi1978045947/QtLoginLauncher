#include "StdAfx.h"
#include <process.h>
#include "HttpThread.h"
#include "LoginClient.h"

HttpThread::HttpThread(LoginClient* loginClient)
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
	m_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_thread = (HANDLE)::_beginthreadex(NULL, 0, &HttpThread::ThreadEntry, this, 0, NULL);
}

HttpThread::~HttpThread()
{
	m_running = false;
	::SetEvent(m_event);
	::WaitForSingleObject(m_thread, INFINITE);
	::CloseHandle(m_thread);
	::CloseHandle(m_event);
	delete m_request;
}

int HttpThread::GetState()
{
	return m_state;
}

int HttpThread::ProcessRequest(HttpRequest* request)
{
	if (m_state == STATE_BUSY)
	{
		return -1;
	}
	m_state = STATE_BUSY;
	m_request = request;
	m_request->userData = m_inUserData;
	::SetEvent(m_event);
	return 0;
}

void HttpThread::ProxyEnable(int enable)
{
	m_proxyEnable = (enable!=0);
}

unsigned _stdcall HttpThread::ThreadEntry(void* param)
{
	HttpThread* pThis = (HttpThread*)param;
	return pThis->ProcessThread();
}

unsigned HttpThread::ProcessThread()
{
	while (m_running)
	{
		WaitForSingleObject(m_event, INFINITE);
		HttpRequest* request = m_request;
		m_request = NULL;
		if (request != NULL)
		{
			string response;
			vector<string> vecCookies;

			int ret = 0;

			bool isUrl2 = false;

			// NOTE:
			// isUrlFailed 是线程级的历史状态，而不是“当前接口/当前主域”的失败结果。
			//
			// 设计策略说明：
			// - 系统中可能存在【多个不同的主域名】（不同接口对应不同主域）。
			// - 一旦任意一个主域名在首次请求中失败，
			//   后续即使是【另一个主域名】的接口，也不再尝试其主域，
			//   而是直接走备用域名（failover 优先，稳定优先）。
			//
			// 这是一个【全流程保守型 failover 策略】，
			// 目的是在登录 / 鉴权 / 短信等强流程中，
			// 避免反复探测主域导致的耗时、抖动或用户体验下降。
			static bool isUrlFailed = false;
			static string publicKey = "";
			static bool isHostBackup3or4 = false;

// 当有host3和host4时候，host1和host2尝试失败则重新试host3和host4
HOST_BACKUP:
			int count = 0;

			if (!isUrlFailed)
			{
				// https连接
				ret = m_httpClient.SendHttpRequest(request->hostName, request->port,
					request->url, request->method, request->postData,
					request->timeout, response, vecCookies, m_proxyEnable);

				count++;

				isUrlFailed = (ret != 0);
			}

			if (request->requestCode != REQ_SendPushMessageVerifyCheckCode && m_loginClient)
			{
				m_loginClient->ClearPushMessageVerifyCheckCodeStatus();
			}
			
			if (isUrlFailed && request->requestCode == REQ_GetDynamicKey && publicKey.length() == 0)
			{
				// http，并先获取publickey
				int r = GetPublicKey(publicKey, response, vecCookies, m_proxyEnable);

				if (r != 0 || publicKey.length() == 0)
				{
					if (!isHostBackup3or4)
					{ 
						// 初始化host3和host4状态
						request->hostName = m_hostName3;
						request->port = m_hostPort3;
						request->hostName2 = m_hostName4;
						request->port2 = m_hostPort4;
						isUrlFailed = false;
						ret = 0;
						publicKey = "";
						response = "";
						m_loginClient->SetHost3AndHost4(m_hostName3, m_hostPort3, m_hostName4, m_hostPort4);
						isHostBackup3or4 = true;
						goto HOST_BACKUP;
					}

					// publicKey获取失败
					m_state = STATE_IDLE;
					m_outUserData = request->userData;
					m_loginClient->ProcessResponse(r, request->requestCode, response, vecCookies, isUrl2);

					delete request;

					continue;
				}
				else
				{
					m_outUserData = request->userData;

					delete request;
					response = "";
					vecCookies.clear();
					request = m_loginClient->GetDynamicKeyRequest(publicKey);

					request->userData = m_outUserData;
				}
			}
						
			if(count == 0)
			{

				string url = request->url2.length() > 0 ? request->url2 : request->url;

				// 如果url失败，那么都都用url2，认证
				ret = m_httpClient.SendHttpRequest(request->hostName2, request->port2,
					url, request->method, request->postData,
					request->timeout2, response, vecCookies, m_proxyEnable);
				isUrl2 = true;
			}

			if (ret != 0 && !request->hostName2.empty())
			{
				string url = request->url2.length() > 0? request->url2:request->url;
				ret = m_httpClient.SendHttpRequest(request->hostName2, request->port2,
					url, request->method, request->postData,
					request->timeout2, response, vecCookies, m_proxyEnable);
				isUrl2 = true;
			}

			m_state = STATE_IDLE;
			m_outUserData = request->userData;
			m_loginClient->ProcessResponse(ret, request->requestCode, response, vecCookies, isUrl2);

			delete request;
		}
	}
	return 0;
}

int HttpThread::Cancel()
{
	if(m_state = STATE_BUSY)
	{
		return m_httpClient.Cancel();
	}

	return 0;
}

int HttpThread::GetPublicKey(string& publicKey, string& response, vector<string>& vecCookies, bool proxyEnable)
{
	// 没有PublicKey没有获取过，先获取一下
	HttpRequest* publicKeyReq = m_loginClient->GetPublicKeyRequest();

	int ret = m_httpClient.SendHttpRequest(publicKeyReq->hostName2, publicKeyReq->port2,
		publicKeyReq->url2, publicKeyReq->method, publicKeyReq->postData,
		publicKeyReq->timeout2, response, vecCookies, m_proxyEnable);

	if (ret != 0)
	{
		ret = m_httpClient.SendHttpRequest(publicKeyReq->hostName2, publicKeyReq->port2,
			publicKeyReq->url2, publicKeyReq->method, publicKeyReq->postData,
			publicKeyReq->timeout2, response, vecCookies, m_proxyEnable);
	}

	if (ret == 0)
	{
		map<string, string> keyValues;
		ResponseProcess::Process(m_loginClient, ret, REQ_GetPublicKey, response, vecCookies, keyValues);

		m_loginClient->ParesePublicKey(ret, keyValues, publicKey);
	}

	return ret;
}

void HttpThread::SetUserData( const void* userData )
{
	m_inUserData = (void*)userData;
}

void* HttpThread::GetUserData()
{
	return m_outUserData;
}
