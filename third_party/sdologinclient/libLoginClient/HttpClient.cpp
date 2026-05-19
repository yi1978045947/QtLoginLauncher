#include "StdAfx.h"
#include "HttpClient.h"
#include "Tracer.h"


HttpClient::HttpClient()
{
	m_connEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_requestEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_compliteEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_step = STEP_NULL;
}

HttpClient::~HttpClient()
{
	::CloseHandle(m_connEvent);
	::CloseHandle(m_requestEvent);
	::CloseHandle(m_compliteEvent);
}

void HttpClient::ResetEvent()
{
	::ResetEvent(m_connEvent);
	::ResetEvent(m_requestEvent);
	::ResetEvent(m_compliteEvent);
}

int HttpClient::SendHttpRequest(const string& hostName, int port,
						const string& urlPath, const string& method,
						const string& urlParam, int timeout,
						string& response, vector<string>& vecCookies,
						bool proxyEnable)
{
	TRACET();
	ResetEvent();

	DWORD proxyFlag;
	if (proxyEnable)
		proxyFlag = INTERNET_OPEN_TYPE_PRECONFIG;
	else
		proxyFlag = INTERNET_OPEN_TYPE_DIRECT;
	string ua = "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; Trident/7.0; rv:11.0) like Gecko\r\n";
	HttpAddRequestHeadersA(m_hRequest, ua.c_str(), (DWORD)-1,
		HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);

	if (m_hInternet == NULL)
	{
		// printf("InternetOpenA wait io pending timeout %d.\n", timeout);
		TRACEE(L"InternetOpenA wait io pending timeout %d.error = %d,return = -7\n",timeout,GetLastError());
		m_step = STEP_NULL;
		return -7;
	}

	::InternetSetStatusCallbackA(m_hInternet, &HttpClient::InternetStatusCallback);
	::InternetSetOptionA(m_hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT,
		(LPVOID)&timeout, sizeof(timeout));

	m_step = STEP_CONN;
    m_hConnect = ::InternetConnectA(m_hInternet, hostName.c_str(), port,
						   NULL, NULL, INTERNET_SERVICE_HTTP, 0, (DWORD_PTR)this);

	try
	{
		if (m_hConnect == NULL && ::GetLastError() == ERROR_IO_PENDING)
		{
			printf("InternetConnect wait io pending.\n");

			if (::WaitForSingleObject(m_connEvent, timeout) != 0)
			{
				//printf("InternetConnect wait io pending timeout %d.\n", timeout);
				TRACEE(L"InternetConnect wait io pending timeout %d.error = %d,return = -1\n",timeout,GetLastError());

				m_step = STEP_NULL;
				::InternetSetStatusCallbackA(m_hInternet, NULL);
				::InternetCloseHandle(m_hInternet);
				return -1;
			}

			if (m_step == STEP_CANCEL)
			{
				throw FALSE;
			}
		}
	}
	catch (BOOL )
	{
		//printf("InternetConnect failed. User Cancel.\n");
		TRACEE(L"InternetConnect failed. User Cancel.error = %d,return = -1\n",GetLastError());
		m_step = STEP_NULL;
		::InternetSetStatusCallbackA(m_hInternet, NULL);
		::InternetCloseHandle(m_hInternet);
		return -1;
	}
	
	if (m_hConnect == NULL)
	{
		//printf("InternetConnect failed. error %d.\n", GetLastError());
		TRACEE(L"InternetConnect failed.error = %d,return = -2\n",GetLastError());

		m_step = STEP_NULL;
		::InternetSetStatusCallbackA(m_hInternet, NULL);
		::InternetCloseHandle(m_hInternet);
		return -2;
	}

	m_step = STEP_REQ;
	DWORD dwFlag = INTERNET_FLAG_RELOAD;
	if (port == INTERNET_DEFAULT_HTTPS_PORT)
		dwFlag  = INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
	m_hRequest = HttpOpenRequestA(m_hConnect, method.c_str(),
		urlPath.c_str(), "HTTP/1.1", NULL, NULL, dwFlag, (DWORD_PTR)this);

	try
	{
		if (m_hRequest == NULL && ::GetLastError() == ERROR_IO_PENDING)
		{
			printf("HttpOpenRequest wait io pending.\n");

			if (::WaitForSingleObject(m_requestEvent, timeout) != 0)
			{
				//printf("HttpOpenRequest wait io pending timeout %d.\n", timeout);

				TRACEE(L"HttpOpenRequest wait io pending timeout %d.error = %d,return = -3\n",timeout,GetLastError());

				m_step = STEP_NULL;
				::InternetSetStatusCallbackA(m_hConnect, NULL);
				::InternetSetStatusCallbackA(m_hInternet, NULL);
				::InternetCloseHandle(m_hConnect);
				::InternetCloseHandle(m_hInternet);
				return -3;
			}
			
			if (m_step == STEP_CANCEL)
			{
				throw FALSE;
			}
		}
	}
	catch (BOOL )
	{
		//printf("HttpOpenRequest failed. User Cancel.\n");
		TRACEE(L"HttpOpenRequest failed. User Cancel.error = %d,return = -3\n",GetLastError());

		m_step = STEP_NULL;
		::InternetSetStatusCallbackA(m_hConnect, NULL);
		::InternetSetStatusCallbackA(m_hInternet, NULL);
		::InternetCloseHandle(m_hConnect);
		::InternetCloseHandle(m_hInternet);
		return -3;
	}
	
	
	if (m_hRequest == NULL)
	{
		printf("HttpOpenRequest failed. error %d.\n", GetLastError());

		m_step = STEP_NULL;
		::InternetSetStatusCallbackA(m_hConnect, NULL);
		::InternetSetStatusCallbackA(m_hInternet, NULL);
		::InternetCloseHandle(m_hConnect);
		::InternetCloseHandle(m_hInternet);
		return -4;
	}

	m_step = STEP_SEND;
	BOOL bRet = ::HttpSendRequest(m_hRequest, NULL, 0,
		(char*)urlParam.c_str(), (DWORD)urlParam.length());
	
	//printf("HttpSendRequest. code %d.\n", GetLastError());
	if(GetLastError() != 0){
		TRACEE(L"HttpSendRequest,error = %d\n",GetLastError());
	}

	try
	{
		if (::WaitForSingleObject(m_compliteEvent, timeout) != 0)
		{
			//printf("HttpSendRequest timeout. error %d.\n", GetLastError());

			TRACEE(L"HttpSendRequest timeout.error = %d,return =-5\n",GetLastError());

			m_step = STEP_NULL;
			::InternetSetStatusCallbackA(m_hRequest, NULL);
			::InternetSetStatusCallbackA(m_hConnect, NULL);
			::InternetSetStatusCallbackA(m_hInternet, NULL);
			::InternetCloseHandle(m_hRequest);
			::InternetCloseHandle(m_hConnect);
			::InternetCloseHandle(m_hInternet);
			return -5;
		}
		if (m_step == STEP_CANCEL)
		{
			throw FALSE;
		}
	}
	catch (BOOL )
	{
		//printf("HttpSendRequest timeout. User Cancel.\n");
		TRACEE(L"HttpSendRequest timeout. User Cancel.error = %d,return =-5\n",GetLastError());

		m_step = STEP_NULL;
		::InternetSetStatusCallbackA(m_hRequest, NULL);
		::InternetSetStatusCallbackA(m_hConnect, NULL);
		::InternetSetStatusCallbackA(m_hInternet, NULL);
		::InternetCloseHandle(m_hRequest);
		::InternetCloseHandle(m_hConnect);
		::InternetCloseHandle(m_hInternet);
		return -5;
	}
	

	DWORD dwStatusCode = 0, dwStatusSize = sizeof(DWORD);
	BOOL nRet = ::HttpQueryInfoA(m_hRequest,
		HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
		&dwStatusCode, &dwStatusSize, 0); 
	if (!nRet || dwStatusCode != 200)
	{
		//printf("HttpQueryInfo. statecode %d code %d.\n", dwStatusCode, GetLastError());
		TRACEE(L"HttpQueryInfo. statecode %d.error = %d,return =-6\n",dwStatusCode,GetLastError());


		m_step = STEP_NULL;
		::InternetSetStatusCallbackA(m_hRequest, NULL);
		::InternetSetStatusCallbackA(m_hConnect, NULL);
		::InternetSetStatusCallbackA(m_hInternet, NULL);
		::InternetCloseHandle(m_hRequest);
		::InternetCloseHandle(m_hConnect);
		::InternetCloseHandle(m_hInternet);
		return -6;
	}

	DWORD count = 0;
	do 
	{
		char cookies[2048] = {0};
		dwStatusSize = 2047;
		nRet = ::HttpQueryInfoA(m_hRequest,
			HTTP_QUERY_SET_COOKIE, &cookies, &dwStatusSize, &count);
		if (nRet)
			vecCookies.push_back(cookies);

		if (m_step == STEP_CANCEL)
		{
			break;
		}

	} while (nRet);

	while (m_step != STEP_CANCEL)
	{
		char readBuffer[4096] = {0};
		unsigned long numberOfBytesRead = 0;
		bRet = InternetReadFile(m_hRequest, readBuffer, sizeof(readBuffer)-1, &numberOfBytesRead);
		if (!bRet)
		{
			if (::GetLastError() == ERROR_IO_PENDING)
			{
				if (::WaitForSingleObject(m_compliteEvent, timeout) != 0)
				{
					printf("InternetReadFile wait io pending timeout. %d.\n", timeout);

					response = "";
					break;
				}
			}
			else
			{
				printf("InternetReadFile failed. error %d.\n", GetLastError());

				response = "";
				break;
			}
		}

		if (numberOfBytesRead == 0)
		{
			break;
		}
		response += string(readBuffer, numberOfBytesRead);
	}

	m_step = STEP_NULL;
	::InternetSetStatusCallbackA(m_hRequest, NULL);
	::InternetSetStatusCallbackA(m_hConnect, NULL);
	::InternetSetStatusCallbackA(m_hInternet, NULL);
	::InternetCloseHandle(m_hRequest);
	::InternetCloseHandle(m_hConnect);
	::InternetCloseHandle(m_hInternet);
	//::HttpEndRequestA(m_hRequest,NULL,NULL,NULL);

	return 0;
}

void _stdcall HttpClient::InternetStatusCallback(
						HINTERNET hInternet, DWORD_PTR dwContext,
						DWORD dwInternetStatus, LPVOID lpvStatusInformation,
						DWORD dwStatusInformationLength)
{
	HttpClient* pThis = (HttpClient*)dwContext;
	pThis->InternetStatusCallback(dwInternetStatus, lpvStatusInformation,
		dwStatusInformationLength);
}

void HttpClient::InternetStatusCallback(
	DWORD dwInternetStatus, LPVOID lpvStatusInformation,
	DWORD dwStatusInformationLength)
{
	INTERNET_ASYNC_RESULT *result = (INTERNET_ASYNC_RESULT*)lpvStatusInformation;

	if (m_step == STEP_CONN)
	{
		if (dwInternetStatus == INTERNET_STATUS_HANDLE_CREATED)
		{
			m_hConnect = (HINTERNET)result->dwResult;
			SetEvent(m_connEvent);
		}else{
			TRACEE(L"STEP_CONN failed,dwInternetStatus=%d\n",dwInternetStatus);
		}
	}
	else if (m_step == STEP_REQ)
	{
		if (dwInternetStatus == INTERNET_STATUS_HANDLE_CREATED)
		{
			m_hRequest = (HINTERNET)result->dwResult;
			SetEvent(m_requestEvent);
		}else{
			TRACEE(L"STEP_REQ failed,dwInternetStatus=%d\n",dwInternetStatus);
		}
	}
	else if (m_step == STEP_SEND)
	{
		if (dwInternetStatus == INTERNET_STATUS_REQUEST_COMPLETE)
		{
			SetEvent(m_compliteEvent);
		}else{
			TRACEE(L"STEP_SEND failed,dwInternetStatus=%d\n",dwInternetStatus);
		}
	}
}

int HttpClient::Cancel()
{
	m_step = STEP_CANCEL;
	if (m_connEvent)
	{
		SetEvent(m_connEvent);
	}
	
	if (m_requestEvent)
	{
		SetEvent(m_requestEvent);
	}

	if (m_compliteEvent)
	{
		SetEvent(m_compliteEvent);
	}

	return 0;
}

