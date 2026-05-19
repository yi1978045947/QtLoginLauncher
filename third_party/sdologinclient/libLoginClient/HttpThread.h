#pragma once

#include "MsgProcess.h"
#include "HttpClient.h"

class LoginClient;

class HttpThread
{
public:
	enum{ STATE_IDLE, STATE_BUSY };

	HttpThread(LoginClient* loginClient);
	~HttpThread();

	int GetState();
	int ProcessRequest(HttpRequest* request);
	void ProxyEnable(int enable);

	int Cancel();

	void SetUserData(const void* userData);
	void* GetUserData();
	void SetState(int state){m_state = state;}

	void SetHost3(string hostname3, int hostport3){m_hostName3 = hostname3; m_hostPort3 = hostport3;};
	void SetHost4(string hostname4, int hostport4){m_hostName4 = hostname4; m_hostPort4 = hostport4;};
	//新版域名关于下行短信
	void SetHost5(string hostname5, int hostport5){m_hostName5 = hostname5; m_hostPort5 = hostport5;};
	void SetHost6(string hostname6, int hostport6){m_hostName6 = hostname6; m_hostPort6 = hostport6;};
private:
	static unsigned _stdcall ThreadEntry(void* param);
	unsigned ProcessThread();

	int GetPublicKey(string& publicKey, string& response, vector<string>& vecCookies, bool proxyEnable);

private:
	HANDLE m_thread;
	HANDLE m_event;
	int m_state;
	bool m_running;
	LoginClient* m_loginClient;
	HttpClient m_httpClient;
	HttpRequest* m_request;
	bool m_proxyEnable;

	void* m_inUserData;
	void* m_outUserData;

	string m_hostName3;
	int m_hostPort3;
	string m_hostName4;
	int m_hostPort4;
	//新版域名关于下行短信
	string m_hostName5;
	int m_hostPort5;
	string m_hostName6;
	int m_hostPort6;
};
