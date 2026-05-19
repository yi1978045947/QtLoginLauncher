#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <process.h>

#include "CurlHttpClient.h"
#include "MsgProcess.h"   // 原项目的 HttpRequest / HttpResponse 结构

class LoginClient;

/**
 * CurlHttpThread
 * ------------------------------------------------------------
 * 模拟原 WinInet 版本的 HttpThread，但内部使用 CurlHttpClient。
 * 主线程
   ↓ 调用 ProcessRequest()
    CurlHttpThread（独立线程）
   ↓ 阻塞式执行 CurlHttpClient.SendHttpRequest
   ↓ 返回 LoginClient::ProcessResponse
 *
 * 主要职责：
 *   - 维护一个后台线程执行 HTTP 请求（防止阻塞主线程）
 *   - 保留原有 HttpThread 所有行为（状态机、事件、Cancel、域名轮询等）
 *   - 保持 LoginClient 调用逻辑不变，做到“无侵入迁移”
 *
 * 工作方式：
 *   LoginClient → 调用 ProcessRequest() → 设置请求 → SetEvent → 启动线程处理
 *   Thread 内部 → 调用 CurlHttpClient.SendHttpRequest → 获取数据 → 再回调给 LoginClient
 *
 * 替换目标：
 *   不再使用 WinInet，转为 libcurl（不会依赖 TLS 开关 / 代理设置 / 防火墙）
 * 
 */
class CurlHttpThread
{
public:
    enum { STATE_IDLE, STATE_BUSY };

    CurlHttpThread(LoginClient* loginClient);
    ~CurlHttpThread();

    /**
     * 获取当前线程状态：空闲 or 正在处理
     */
    int GetState();

    /**
     * 提交请求给线程（与旧接口保持一致）
     * 内部会把 request 保存起来，并激活线程
     */
    int ProcessRequest(HttpRequest* request);

    /**
     * 设置是否启用代理（某些游戏场景必须禁用代理）
     */
    void ProxyEnable(int enable);

    /**
     * 强制取消当前 HTTP 请求
     * Cancel 会传递给 CurlHttpClient
     */
    int Cancel();

    /**
     * 用来传递任意用户数据（同原版）
     */
    void SetUserData(const void* userData);
    void* GetUserData();

    void SetState(int state) { m_state = state; }

    /** 以下 4 个接口：保留原逻辑的域名轮询结构 */

    void SetHost3(std::string hostname3, int hostport3)
    {
        m_hostName3 = hostname3; 
        m_hostPort3 = hostport3;
    }

    void SetHost4(std::string hostname4, int hostport4)
    {
        m_hostName4 = hostname4; 
        m_hostPort4 = hostport4;
    }

    void SetHost5(std::string hostname5, int hostport5)
    {
        m_hostName5 = hostname5; 
        m_hostPort5 = hostport5;
    }

    void SetHost6(std::string hostname6, int hostport6)
    {
        m_hostName6 = hostname6; 
        m_hostPort6 = hostport6;
    }

    void SetHost7(std::string hostname7, int hostport7)
    {
        m_hostName7 = hostname7;
        m_hostPort7 = hostport7;
    }

    void SetHost8(std::string hostname8, int hostport8)
    {
        m_hostName8 = hostname8;
        m_hostPort8 = hostport8;
    }

    /**
     * 设置是否启用 HTTPS 证书验证
     * 默认为 true（生产模式）
     * 调试抓包时可关闭：false
     */
    /*
    *   LoginClient / SDK 初始化时
        ↓
        CurlHttpThread::SetVerifyCert()
        ↓
        CurlHttpClient::SetVerifyCert()
        ↓
        SendHttpRequest() 里根据 m_verifyCert 决定是否验证证书
    */
    void SetVerifyCert(bool enable);

    /**
     * IsCDomainRequest
     * ------------------------------------------------------------
     * 判断某个 requestCode 是否属于 C 域名组（mgame.sdo.com）。
     *
     * 设计目的：
     *   - 将“域名路由策略”与线程执行逻辑解耦
     *   - 避免在 ProcessThread() 中写大量 requestCode 判断
     *   - 便于未来新增 mgame.sdo.com 组接口时集中维护
     *
     * 行为说明：
     *   - 返回 true  → 该请求固定走 host5/host6
     *                  不参与 host3/host4 备用切换机制
     *                  不受 g_hasSwitchedToBackupGroup 影响
     *
     *   - 返回 false → 该请求属于默认 cas.sdo.com 组
     *                  参与原有 host3/host4 容灾机制
     *
     * 注意：
     *   - 这里只做“分组判断”，不做任何网络操作
     *   - 不修改 request 对象，仅作为逻辑判断入口
     *
     *   - 未来若支持多组域名（例如支付域名组）
     *     可以扩展为：
     *         GetDomainGroup(requestCode)
     *
     */
    bool IsCDomainRequest(int requestCode);
private:
    /**
     * ThreadEntry
     * ------------------------------------------------------------
     * Windows 线程入口，固定写法，由 _beginthreadex 调用。
     * 内部转发到类成员函数 ProcessThread。
     */
    static unsigned __stdcall ThreadEntry(void* param);

    /**
     * ProcessThread
     * ------------------------------------------------------------
     * 实际后台线程逻辑：
     *   - 等待事件（m_event）
     *   - 收到请求后，执行 HTTP 请求
     *   - 将结果回调给 LoginClient
     */
    unsigned ProcessThread();

    /**
     * GetPublicKey
     * ------------------------------------------------------------
     * 获取公钥（旧 HttpThread 中的逻辑迁移）
     * 某些请求必须先获取动态 key，这里沿用旧流程
     */
    int GetPublicKey(std::string& publicKey,
        std::string& response,
        std::vector<std::string>& vecCookies,
        bool proxyEnable);

private:
    HANDLE m_thread;       ///< 后台线程句柄
    HANDLE m_event;        ///< 激活线程的事件
    int    m_state;        ///< 状态：空闲 / 忙碌
    bool   m_running;      ///< 线程运行标志

    LoginClient* m_loginClient; ///< 上层业务对象（原样保留）

    CurlHttpClient m_httpClient; ///< 用 curl 实现 HTTP 请求

    HttpRequest* m_request; ///< 当前正在处理的请求

    bool m_proxyEnable;     ///< 是否启用代理

    void* m_inUserData;     ///< 上层传入数据
    void* m_outUserData;    ///< 上层传出数据

    /** 备用域名机制（原样保留） */
    std::string m_hostName3; int m_hostPort3;
    std::string m_hostName4; int m_hostPort4;
    std::string m_hostName5; int m_hostPort5;
    std::string m_hostName6; int m_hostPort6;

    std::string m_hostName7; int m_hostPort7;
    std::string m_hostName8; int m_hostPort8;
};

