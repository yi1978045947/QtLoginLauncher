#pragma once

class HttpClient
{
public:
	HttpClient();
	~HttpClient();

	int SendHttpRequest(const string& hostName, int port,
		const string& url, const string& method,
		const string& postData, int timeout,
		string& response, vector<string>& vecCookies, bool proxyEnable);

	int Cancel();

private:
	void ResetEvent();
	static void _stdcall InternetStatusCallback(
		HINTERNET hInternet, DWORD_PTR dwContext,
		DWORD dwInternetStatus, LPVOID lpvStatusInformation,
		DWORD dwStatusInformationLength);
	void InternetStatusCallback(
		DWORD dwInternetStatus, LPVOID lpvStatusInformation,
		DWORD dwStatusInformationLength);

private:
	enum {STEP_NULL, STEP_CONN, STEP_REQ, STEP_CANCEL, STEP_SEND};

	HANDLE		m_connEvent;
	HANDLE		m_requestEvent;
	HANDLE		m_compliteEvent;
	HINTERNET	m_hInternet;
	HINTERNET	m_hConnect;
	HINTERNET	m_hRequest;
	int			m_step;
};
