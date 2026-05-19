#include "sdk_runtime.h"

#include <iterator>
#include <sstream>

#include "logger.h"
#include "string_util.h"
#include "named_pipe.h"
#include "protocol_types.h"
#include "login_process_manager.h"
#include "directx_compat_adapter.h"
#include "process_util.h"

namespace qtlogin::sdk {

namespace {

std::string narrowInt(int value)
{
    return std::to_string(value);
}

std::string narrowPointer(HWND window)
{
    std::ostringstream stream;
    stream << reinterpret_cast<uintptr_t>(window);
    return stream.str();
}

std::string narrowDouble(double value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

double legacyDpiScaleForOwnerWindow(HWND owner)
{
    UINT dpi = 0;
    if (owner) {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
        auto* getDpiForWindow = user32
            ? reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(user32, "GetDpiForWindow"))
            : nullptr;
        if (getDpiForWindow) {
            dpi = getDpiForWindow(owner);
        }
    }

    if (dpi == 0) {
        HDC screen = GetDC(nullptr);
        if (screen) {
            dpi = static_cast<UINT>(GetDeviceCaps(screen, LOGPIXELSX));
            ReleaseDC(nullptr, screen);
        }
    }

    if (dpi == 0) {
        dpi = 96;
    }
    return static_cast<double>(dpi) / 96.0;
}

int parseErrorCode(const std::string& value)
{
    try {
        return std::stoi(value);
    } catch (...) {
        return SDOL_ERRORCODE_FAILED;
    }
}

std::string fieldOrEmpty(const qtlogin::protocol::Message& message, const char* key)
{
    const auto it = message.fields.find(key);
    return it == message.fields.end() ? std::string() : it->second;
}

std::wstring sdkModuleDirectory()
{
    HMODULE module = nullptr;
    if (GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&sdkModuleDirectory),
            &module)) {
        return common::moduleDirectory(module);
    }
    return common::currentExecutableDirectory();
}

bool browserLaunchDisabled()
{
    wchar_t value[8] = {};
    const DWORD length = GetEnvironmentVariableW(L"QTLOGIN_DISABLE_BROWSER_LAUNCH", value, static_cast<DWORD>(std::size(value)));
    return length > 0 && length < std::size(value);
}

int launchBrowserWindow(const std::wstring& title, const std::wstring& url)
{
    if (url.empty()) {
        return SDOL_ERRORCODE_INVALIDPARAM;
    }

    const std::wstring directory = sdkModuleDirectory();
    const std::wstring exePath = common::joinPath(directory, L"qtlogin_browser.exe");
    std::wstring commandLine = common::quoteForCommandLine(exePath);
    commandLine += L" ";
    commandLine += common::quoteForCommandLine(L"--title=" + (title.empty() ? std::wstring(L"SDK Web Window") : title));
    commandLine += L" ";
    commandLine += common::quoteForCommandLine(L"--url=" + url);

    if (browserLaunchDisabled()) {
        common::logLine(L"sdk", L"browser launch disabled for test: " + commandLine);
        return SDOL_ERRORCODE_OK;
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::wstring mutableCommandLine = commandLine;
    const BOOL created = CreateProcessW(
        exePath.c_str(),
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_PROCESS_GROUP,
        nullptr,
        directory.c_str(),
        &startup,
        &process);
    if (!created) {
        common::logLine(L"sdk", L"failed to launch browser error=" + std::to_wstring(GetLastError()));
        return SDOL_ERRORCODE_FAILED;
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return SDOL_ERRORCODE_OK;
}

std::string jsonField(const char* json, const char* key)
{
    if (!json || !key) {
        return std::string();
    }

    const std::string text(json);
    const std::string quotedKey = std::string("\"") + key + "\"";
    size_t pos = text.find(quotedKey);
    if (pos == std::string::npos) {
        return std::string();
    }
    pos = text.find(':', pos + quotedKey.size());
    if (pos == std::string::npos) {
        return std::string();
    }
    ++pos;
    while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\"')) {
        ++pos;
    }

    size_t end = pos;
    while (end < text.size() && text[end] != '\"' && text[end] != ',' && text[end] != '}') {
        ++end;
    }
    return text.substr(pos, end - pos);
}

std::wstring buildPaymentUrl(int appId, const char* extend)
{
    const std::string explicitUrl = jsonField(extend, "url");
    if (!explicitUrl.empty()) {
        return common::utf8ToWide(explicitUrl);
    }

    const std::wstring productId = common::utf8ToWide(jsonField(extend, "productid").empty() ? "GWPAY-TEST" : jsonField(extend, "productid"));
    const std::wstring areaId = common::utf8ToWide(jsonField(extend, "areaid").empty() ? "1" : jsonField(extend, "areaid"));
    const std::wstring groupId = common::utf8ToWide(jsonField(extend, "groupid").empty() ? "1" : jsonField(extend, "groupid"));
    const std::wstring gameOrder = common::utf8ToWide(jsonField(extend, "gameorder").empty() ? "ORDER-TEST" : jsonField(extend, "gameorder"));

    return L"https://cas.sdo.com/authen/caslogin?gateway=true&appId="
        + std::to_wstring(appId)
        + L"&service=https%3A%2F%2Fpaysdbx.u.sdo.com%2Fitem%2F"
        + productId
        + L"%3Fgm%3D"
        + groupId
        + L"%26area%3D"
        + areaId
        + L"%26gameOrderNo%3D"
        + gameOrder;
}

HANDLE duplicateProcessHandle(HANDLE process)
{
    if (!process) {
        return nullptr;
    }

    HANDLE duplicate = nullptr;
    if (!DuplicateHandle(
            GetCurrentProcess(),
            process,
            GetCurrentProcess(),
            &duplicate,
            PROCESS_TERMINATE | SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION,
            FALSE,
            0)) {
        return nullptr;
    }
    return duplicate;
}

void stopAndCloseProcess(HANDLE process)
{
    if (!process) {
        return;
    }

    DWORD exitCode = 0;
    if (GetExitCodeProcess(process, &exitCode) && exitCode == STILL_ACTIVE) {
        TerminateProcess(process, 0);
    }
    WaitForSingleObject(process, 2000);
    CloseHandle(process);
}

}

SdkRuntime& SdkRuntime::instance()
{
    static SdkRuntime runtime;
    return runtime;
}

std::wstring SdkRuntime::safeString(LPCWSTR value)
{
    return value ? std::wstring(value) : std::wstring();
}

std::wstring SdkRuntime::makePipeName(uint64_t requestId)
{
    std::wstringstream stream;
    stream << L"QtLoginRewrite_" << GetCurrentProcessId() << L"_" << GetTickCount64() << L"_" << requestId;
    return stream.str();
}

int SdkRuntime::copyAppInfo(const SDOLAppInfo* appInfo)
{
    if (!appInfo || appInfo->Size < sizeof(SDOLAppInfo)) {
        return SDOL_ERRORCODE_INVALIDPARAM;
    }

    appInfo_.size = appInfo->Size;
    appInfo_.appId = appInfo->AppID;
    appInfo_.appName = safeString(appInfo->AppName);
    appInfo_.appVer = safeString(appInfo->AppVer);
    appInfo_.areaId = appInfo->AreaId;
    appInfo_.groupId = appInfo->GroupId;
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::initialize(const SDOLAppInfo* appInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const int result = copyAppInfo(appInfo);
    if (result != SDOL_ERRORCODE_OK) {
        return result;
    }

    initialized_ = true;
    lastSessionId_.clear();
    lastSndaid_.clear();
    lastIdentityState_.clear();
    lastTicket_.clear();
    common::logLine(L"sdk", L"initialized");
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::terminal()
{
    HANDLE processToStop = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        processToStop = activeLoginProcess_;
        activeLoginProcess_ = nullptr;
        activeLoginRequestId_ = 0;
        releaseDirectXCompatHooks();
        initialized_ = false;
        ownerWindow_ = nullptr;
        loginMode_ = NormalLoginMode;
        directXOwner_ = false;
        lastSessionId_.clear();
        lastSndaid_.clear();
        lastIdentityState_.clear();
        lastTicket_.clear();
    }

    stopAndCloseProcess(processToStop);
    common::logLine(L"sdk", L"terminal");
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::setOwnerWindow(HWND window)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ownerWindow_ = window;
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::markDirectXOwnerWindow(HWND window)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ownerWindow_ = window;
    directXOwner_ = window != nullptr;
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::setLoginMode(int mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    loginMode_ = mode;
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::modifyAppInfo(const SDOLAppInfo* appInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return copyAppInfo(appInfo);
}

int SdkRuntime::setGameClientType(LPCWSTR gameClientType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    gameClientType_ = safeString(gameClientType);
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::modifyServerId(LPCSTR serverId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    serverId_ = serverId ? std::string(serverId) : std::string();
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::closeLoginDialog()
{
    HANDLE processToStop = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        processToStop = activeLoginProcess_;
        activeLoginProcess_ = nullptr;
        activeLoginRequestId_ = 0;
    }

    stopAndCloseProcess(processToStop);
    common::logLine(L"sdk", L"close login dialog requested");
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::moveLoginDialog(int x, int y)
{
    std::lock_guard<std::mutex> lock(mutex_);
    loginPosX_ = x;
    loginPosY_ = y;
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::logout()
{
    std::lock_guard<std::mutex> lock(mutex_);
    lastSessionId_.clear();
    lastSndaid_.clear();
    lastIdentityState_.clear();
    lastTicket_.clear();
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::showLoginDialog(LPSDOLLOGINCALLBACKPROC callback, int userData, int reserved)
{
    AppInfoCopy appInfo;
    HWND owner = nullptr;
    int loginMode = NormalLoginMode;
    bool directXOwner = false;
    int loginPosX = 0;
    int loginPosY = 0;
    std::wstring gameClientType;
    std::string serverId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) {
            return SDOL_ERRORCODE_FAILED;
        }
        appInfo = appInfo_;
        owner = ownerWindow_;
        loginMode = loginMode_;
        directXOwner = directXOwner_;
        loginPosX = loginPosX_;
        loginPosY = loginPosY_;
        gameClientType = gameClientType_;
        serverId = serverId_;
    }

    const uint64_t requestId = nextRequestId_.fetch_add(1);
    const std::wstring pipeName = makePipeName(requestId);
    std::wstring extraArguments;
    wchar_t autoSuccessMs[32] = {};
    const DWORD autoSuccessLength = GetEnvironmentVariableW(L"QTLOGIN_AUTO_SUCCESS_MS", autoSuccessMs, static_cast<DWORD>(std::size(autoSuccessMs)));
    if (autoSuccessLength > 0 && autoSuccessLength < std::size(autoSuccessMs)) {
        extraArguments += L" --auto-success-ms=";
        extraArguments += autoSuccessMs;
    }

    LoginProcessManager process;
    if (!process.start(pipeName, extraArguments)) {
        return SDOL_ERRORCODE_FAILED;
    }

    HANDLE activeHandle = duplicateProcessHandle(process.processHandle());
    if (!activeHandle) {
        TerminateProcess(process.processHandle(), 0);
        process.wait(2000);
        return SDOL_ERRORCODE_FAILED;
    }

    HANDLE previousActiveProcess = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        previousActiveProcess = activeLoginProcess_;
        activeLoginProcess_ = activeHandle;
        activeLoginRequestId_ = requestId;
    }
    stopAndCloseProcess(previousActiveProcess);

    auto takeActiveProcess = [this, requestId]() {
        HANDLE process = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (activeLoginRequestId_ == requestId) {
                process = activeLoginProcess_;
                activeLoginProcess_ = nullptr;
                activeLoginRequestId_ = 0;
            }
        }
        return process;
    };
    auto clearActiveProcess = [&takeActiveProcess]() {
        HANDLE processToClose = takeActiveProcess();
        if (processToClose) {
            CloseHandle(processToClose);
        }
    };
    auto failActiveLogin = [&takeActiveProcess](int errorCode) {
        stopAndCloseProcess(takeActiveProcess());
        return errorCode;
    };

    ipc::PipeConnection connection;
    ipc::PipeServer server;
    if (!server.listen(pipeName, 10000, &connection)) {
        common::logLine(L"sdk", L"pipe server listen timeout");
        return failActiveLogin(SDOL_ERRORCODE_FAILED);
    }

    protocol::Message hello;
    if (!connection.receive(&hello) || hello.type != protocol::MessageType::Hello) {
        common::logLine(L"sdk", L"invalid hello from sdologin");
        return failActiveLogin(SDOL_ERRORCODE_FAILED);
    }

    protocol::Message ack;
    ack.type = protocol::MessageType::HelloAck;
    ack.requestId = requestId;
    ack.fields["sdkProcessId"] = narrowInt(static_cast<int>(GetCurrentProcessId()));
    if (!connection.send(ack)) {
        return failActiveLogin(SDOL_ERRORCODE_FAILED);
    }

    protocol::Message show;
    show.type = protocol::MessageType::ShowLogin;
    show.requestId = requestId;
    show.fields["appId"] = narrowInt(appInfo.appId);
    show.fields["appName"] = common::wideToUtf8(appInfo.appName);
    show.fields["appVer"] = common::wideToUtf8(appInfo.appVer);
    show.fields["areaId"] = narrowInt(appInfo.areaId);
    show.fields["groupId"] = narrowInt(appInfo.groupId);
    show.fields["loginMode"] = narrowInt(loginMode);
    show.fields["ownerHwnd"] = narrowPointer(owner);
    show.fields["embedWindow"] = (directXOwner || loginMode == AttachToLoginMode) ? "1" : "0";
    if (show.fields["embedWindow"] == "1") {
        const double dpiScale = legacyDpiScaleForOwnerWindow(owner);
        show.fields["legacyDpiScale"] = narrowDouble(dpiScale);
        common::logLine(L"sdk", L"passing owner dpi scale=" + std::to_wstring(dpiScale));
    }
    show.fields["posX"] = narrowInt(loginPosX);
    show.fields["posY"] = narrowInt(loginPosY);
    show.fields["gameClientType"] = common::wideToUtf8(gameClientType);
    show.fields["serverId"] = serverId;
    show.fields["reserved"] = narrowInt(reserved);
    if (!connection.send(show)) {
        return failActiveLogin(SDOL_ERRORCODE_FAILED);
    }

    protocol::Message result;
    if (!connection.receive(&result)) {
        return failActiveLogin(SDOL_ERRORCODE_LOGINCANCEL);
    }

    int callbackError = SDOL_ERRORCODE_FAILED;
    SDOLLoginResult loginResult{};
    std::wstring sessionId;
    std::wstring sndaid;
    std::wstring identityState;
    std::wstring ticket;

    if (result.type == protocol::MessageType::LoginSuccess) {
        callbackError = SDOL_ERRORCODE_OK;
        sessionId = common::utf8ToWide(fieldOrEmpty(result, "sessionId"));
        sndaid = common::utf8ToWide(fieldOrEmpty(result, "sndaid"));
        identityState = common::utf8ToWide(fieldOrEmpty(result, "identityState"));
        ticket = common::utf8ToWide(fieldOrEmpty(result, "ticket"));

        loginResult.Size = sizeof(loginResult);
        loginResult.SessionId = sessionId.c_str();
        loginResult.Sndaid = sndaid.c_str();
        loginResult.IdentityState = identityState.c_str();
        loginResult.Appendix = L"";

        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastSessionId_ = sessionId;
            lastSndaid_ = sndaid;
            lastIdentityState_ = identityState;
            lastTicket_ = ticket.empty() ? sessionId : ticket;
        }
    } else if (result.type == protocol::MessageType::LoginCancel) {
        callbackError = SDOL_ERRORCODE_LOGINCANCEL;
    } else if (result.type == protocol::MessageType::LoginError) {
        callbackError = parseErrorCode(fieldOrEmpty(result, "errorCode"));
    }

    if (callback) {
        callback(callbackError, callbackError == SDOL_ERRORCODE_OK ? &loginResult : nullptr, userData, 0);
    }

    process.wait(2000);
    clearActiveProcess();
    return callbackError;
}

int SdkRuntime::getTicket(BSTR* ticket, BSTR* sndaid)
{
    if (!ticket || !sndaid) {
        return SDOL_ERRORCODE_INVALIDPARAM;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (lastTicket_.empty() || lastSndaid_.empty()) {
        *ticket = nullptr;
        *sndaid = nullptr;
        return SDOL_ERRORCODE_GETTICKET_TIMEOUT;
    }

    *ticket = SysAllocString(lastTicket_.c_str());
    *sndaid = SysAllocString(lastSndaid_.c_str());
    if (!*ticket || !*sndaid) {
        if (*ticket) {
            SysFreeString(*ticket);
        }
        if (*sndaid) {
            SysFreeString(*sndaid);
        }
        *ticket = nullptr;
        *sndaid = nullptr;
        return SDOL_ERRORCODE_FAILED;
    }
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::recordLoginResult(
    const std::wstring& sessionId,
    const std::wstring& sndaid,
    const std::wstring& identityState,
    const std::wstring&,
    const std::wstring& ticket)
{
    std::lock_guard<std::mutex> lock(mutex_);
    lastSessionId_ = sessionId;
    lastSndaid_ = sndaid;
    lastIdentityState_ = identityState;
    lastTicket_ = ticket.empty() ? sessionId : ticket;
    return SDOL_ERRORCODE_OK;
}

int SdkRuntime::openWindow(LPCWSTR, LPCWSTR winName, LPCWSTR src, int, int, int, int, LPCWSTR)
{
    return launchBrowserWindow(safeString(winName), safeString(src));
}

int SdkRuntime::ghomePay(const char* extend, LPLOGINPAYCALLBACKPROC callback)
{
    int appId = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        appId = appInfo_.appId;
    }

    const std::wstring url = buildPaymentUrl(appId, extend);
    const int result = launchBrowserWindow(L"统一收银台", url);
    if (callback) {
        callback(result, result == SDOL_ERRORCODE_OK ? "payment window opened" : "payment window failed");
    }
    return result;
}

int SdkRuntime::openFaceVerifyDialog(LPFACEVERTIFYCALLBACKPROC callback, LPCWSTR verifyType)
{
    const std::wstring type = safeString(verifyType);
    const std::wstring url = type.rfind(L"http://", 0) == 0 || type.rfind(L"https://", 0) == 0
        ? type
        : L"about:blank";
    const int result = launchBrowserWindow(L"人脸核验", url);
    if (callback) {
        callback(result, result == SDOL_ERRORCODE_OK ? "face_verify_window_opened" : "face_verify_window_failed");
    }
    return result;
}

int SdkRuntime::openCollectUserMsgDialog(LPCOLLECTUSERMSGCALLBACKPROC callback, LPCWSTR collectMsgType)
{
    const std::wstring type = safeString(collectMsgType);
    const std::wstring url = type.rfind(L"http://", 0) == 0 || type.rfind(L"https://", 0) == 0
        ? type
        : L"about:blank";
    const int result = launchBrowserWindow(L"实名资料收集", url);
    if (callback) {
        callback(result, result == SDOL_ERRORCODE_OK ? "collect_user_msg_window_opened" : "collect_user_msg_window_failed");
    }
    return result;
}

}
