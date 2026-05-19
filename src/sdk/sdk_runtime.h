#pragma once

#include <atomic>
#include <mutex>
#include <string>

#include <windows.h>
#include <oleauto.h>

#include "legacy/interface.h"

namespace qtlogin::sdk {

class SdkRuntime {
public:
    static SdkRuntime& instance();

    int initialize(const SDOLAppInfo* appInfo);
    int terminal();
    int setOwnerWindow(HWND window);
    int setLoginMode(int mode);
    int markDirectXOwnerWindow(HWND window);
    int modifyAppInfo(const SDOLAppInfo* appInfo);
    int setGameClientType(LPCWSTR gameClientType);
    int modifyServerId(LPCSTR serverId);
    int closeLoginDialog();
    int moveLoginDialog(int x, int y);
    int logout();
    int showLoginDialog(LPSDOLLOGINCALLBACKPROC callback, int userData, int reserved);
    int getTicket(BSTR* ticket, BSTR* sndaid);
    int recordLoginResult(const std::wstring& sessionId, const std::wstring& sndaid, const std::wstring& identityState, const std::wstring& appendix, const std::wstring& ticket);
    int openWindow(LPCWSTR winType, LPCWSTR winName, LPCWSTR src, int left, int top, int width, int height, LPCWSTR mode);
    int ghomePay(const char* extend, LPLOGINPAYCALLBACKPROC callback);
    int openFaceVerifyDialog(LPFACEVERTIFYCALLBACKPROC callback, LPCWSTR verifyType);
    int openCollectUserMsgDialog(LPCOLLECTUSERMSGCALLBACKPROC callback, LPCWSTR collectMsgType);

private:
    SdkRuntime() = default;

    struct AppInfoCopy {
        DWORD size = sizeof(SDOLAppInfo);
        int appId = 0;
        std::wstring appName;
        std::wstring appVer;
        int areaId = -1;
        int groupId = -1;
    };

    static std::wstring safeString(LPCWSTR value);
    static std::wstring makePipeName(uint64_t requestId);
    int copyAppInfo(const SDOLAppInfo* appInfo);

    std::mutex mutex_;
    bool initialized_ = false;
    HWND ownerWindow_ = nullptr;
    int loginMode_ = NormalLoginMode;
    bool directXOwner_ = false;
    int loginPosX_ = 0;
    int loginPosY_ = 0;
    AppInfoCopy appInfo_;
    std::wstring gameClientType_;
    std::string serverId_;
    std::wstring lastSessionId_;
    std::wstring lastSndaid_;
    std::wstring lastIdentityState_;
    std::wstring lastTicket_;
    std::atomic<uint64_t> nextRequestId_{1};
    HANDLE activeLoginProcess_ = nullptr;
    uint64_t activeLoginRequestId_ = 0;
};

}
