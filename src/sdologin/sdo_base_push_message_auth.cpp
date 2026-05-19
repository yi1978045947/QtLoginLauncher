#include "sdo_base_push_message_auth.h"

#include <cstring>
#include <sstream>
#include <utility>

#include <windows.h>

#include "legacy_password_auth.h"
#include "logger.h"

#if (defined(_M_IX86) || defined(__i386__)) && defined(QTLOGIN_ENABLE_LEGACY_AUTH_LIBS)
#define QTLOGIN_HAS_PUSH_MESSAGE_LEGACY_AUTH_LIBS 1
#include "SdoBaseClient.h"
#else
#define QTLOGIN_HAS_PUSH_MESSAGE_LEGACY_AUTH_LIBS 0
#endif

namespace qtlogin::sdologin {
namespace {

bool isXpOrOlder()
{
    OSVERSIONINFOEXW version{};
    version.dwOSVersionInfoSize = sizeof(version);
#pragma warning(push)
#pragma warning(disable:4996)
    const BOOL ok = GetVersionExW(reinterpret_cast<OSVERSIONINFOW*>(&version));
#pragma warning(pop)
    return ok && version.dwMajorVersion <= 5;
}

PushMessageSendResult sendFailure(int errorCode, std::wstring message)
{
    PushMessageSendResult result;
    result.success = false;
    result.errorCode = errorCode;
    result.errorMessage = std::move(message);
    return result;
}

PushMessageLoginResult loginFailure(int errorCode, std::wstring message)
{
    PushMessageLoginResult result;
    result.success = false;
    result.errorCode = errorCode;
    result.errorMessage = std::move(message);
    return result;
}

PushMessageSendResult unsupportedSendResult()
{
    return sendFailure(-10524100, L"当前构建无法加载旧 32 位 SdoBaseClient，请使用 VS2019 Win32 + 32 位 Qt 测试真实一键登录。");
}

PushMessageLoginResult unsupportedLoginResult()
{
    return loginFailure(-10524100, L"当前构建无法加载旧 32 位 SdoBaseClient，请使用 VS2019 Win32 + 32 位 Qt 测试真实一键登录。");
}

void logPushMessagePhase(const std::wstring& message)
{
    qtlogin::common::logLine(L"push-message-auth", message);
}

}

class SdoBasePushMessageAuthenticator::Impl {
public:
    explicit Impl(SdoBaseAuthConfig config)
        : config_(std::move(config))
    {
    }

    ~Impl()
    {
#if QTLOGIN_HAS_PUSH_MESSAGE_LEGACY_AUTH_LIBS
        cancel();
        if (handle_) {
            SdoBase_Release(handle_);
            handle_ = nullptr;
        }
#endif
    }

    void startLogin(const PushMessageLoginRequest& request, PushMessageSendCompletion completion)
    {
        sendCompletion_ = std::move(completion);
        const PushMessageLoginValidationResult validation = validatePushMessageLoginRequest(request);
        if (validation.code != PushMessageLoginValidationCode::Ok) {
            completeSend(sendFailure(-10524002, validation.message));
            return;
        }
        currentAccountWide_ = validation.trimmedAccount;
        currentAccountAnsi_ = wideToAnsiForLegacyAuth(validation.trimmedAccount);
        sendPushIssuedAfterCancel_ = false;

#if QTLOGIN_HAS_PUSH_MESSAGE_LEGACY_AUTH_LIBS
        if (!ensureInitialized()) {
            completeSend(lastInitializeSendFailure_);
            return;
        }

        SdoBase_SetUserData(handle_, this);
        logPushMessagePhase(L"push-message begin: get dynamic key");
        const int error = callWithBusyRetry([this]() {
            return SdoBase_GetDynamicKey(handle_);
        });
        {
            std::wostringstream out;
            out << L"SdoBase_GetDynamicKey returned " << error;
            logPushMessagePhase(out.str());
        }
        if (error != 0) {
            completeSend(sendFailure(
                mapLegacySdoBaseResultCode(error),
                L"SdoBase_GetDynamicKey call failed for push-message login."));
        }
#else
        completeSend(unsupportedSendResult());
#endif
    }

    void pollLogin(PushMessageLoginCompletion completion)
    {
        loginCompletion_ = std::move(completion);

#if QTLOGIN_HAS_PUSH_MESSAGE_LEGACY_AUTH_LIBS
        if (!ensureInitialized()) {
            completeLogin(lastInitializeLoginFailure_);
            return;
        }

        SdoBase_SetUserData(handle_, this);
        logPushMessagePhase(L"push-message poll begin");
        const int error = SdoBase_PushMessageLogin(handle_, 0, 0, 0);
        {
            std::wostringstream out;
            out << L"SdoBase_PushMessageLogin returned " << error;
            logPushMessagePhase(out.str());
        }
        if (error != 0) {
            completeLogin(loginFailure(
                mapLegacySdoBaseResultCode(error),
                L"SdoBase_PushMessageLogin call failed."));
        }
#else
        completeLogin(unsupportedLoginResult());
#endif
    }

    void cancel()
    {
#if QTLOGIN_HAS_PUSH_MESSAGE_LEGACY_AUTH_LIBS
        if (handle_) {
            SdoBase_Cancel(handle_);
        }
#endif
    }

#if QTLOGIN_HAS_PUSH_MESSAGE_LEGACY_AUTH_LIBS
    static void SDOAPI onGetDynamicKeyCallback(int resultCode, const char* failReason, const char* dynamicKey, const char* guid, SdoBaseHandle* handle)
    {
        auto* self = static_cast<Impl*>(SdoBase_GetUserData(handle));
        if (self) {
            self->handleDynamicKey(resultCode, failReason, dynamicKey, guid);
        }
    }

    static void SDOAPI onCancelPushMessageCallback(int resultCode, const char* failReason, SdoBaseHandle* handle)
    {
        auto* self = static_cast<Impl*>(SdoBase_GetUserData(handle));
        if (self) {
            self->handleCancelPushMessage(resultCode, failReason);
        }
    }

    static void SDOAPI onSendPushMessageCallback(int resultCode, const char* failReason, const char* pushMsgSerialNum, SdoBaseHandle* handle)
    {
        auto* self = static_cast<Impl*>(SdoBase_GetUserData(handle));
        if (self) {
            self->handleSendPushMessage(resultCode, failReason, pushMsgSerialNum);
        }
    }

    static void SDOAPI onLoginResultCallback(int resultCode, const char* failReason, const char* sndaId,
        const char* ticket, const char* accountUpgradeUrl, const char* mobile,
        const char* autoLoginSessionKey, int autoLoginMaxAge, int popWindowFlag,
        const char* redirectURL, const char* inputUserId, const char* bindMid,
        const char* noteName, const char* dispalyName, const char* companyId,
        bool isNew, const char* appMid, const char* tgt, const char* keepLoginKey,
        const char* flowid, const char* isScanned, SdoBaseHandle* handle)
    {
        (void)accountUpgradeUrl;
        (void)mobile;
        (void)autoLoginSessionKey;
        (void)autoLoginMaxAge;
        (void)popWindowFlag;
        (void)redirectURL;
        (void)bindMid;
        (void)noteName;
        (void)dispalyName;
        (void)companyId;
        (void)isNew;
        (void)appMid;
        (void)keepLoginKey;
        (void)flowid;
        (void)isScanned;
        auto* self = static_cast<Impl*>(SdoBase_GetUserData(handle));
        if (self) {
            self->handleLoginResult(resultCode, failReason, sndaId, ticket, tgt, inputUserId);
        }
    }

    static void SDOAPI onDynamicLoginCallback(int resultCode, const char* failReason, int loginType, int deviceType,
        const char* deviceDisplayType, const char* challenge, const char* goDownLoginType,
        const char* dynamicKey, const char* inputUserId, SdoBaseHandle* handle)
    {
        (void)loginType;
        (void)deviceType;
        (void)deviceDisplayType;
        (void)challenge;
        (void)goDownLoginType;
        (void)dynamicKey;
        (void)inputUserId;
        auto* self = static_cast<Impl*>(SdoBase_GetUserData(handle));
        if (!self) {
            return;
        }
        PushMessageLoginResult result;
        result.success = false;
        result.needsSecondFactor = true;
        result.errorCode = mapLegacySdoBaseResultCode(resultCode);
        result.errorMessage = failReason && *failReason
            ? ansiToWideForLegacyAuth(failReason)
            : L"一键登录需要继续二次认证，Qt 重写版后续会接入对应弹窗。";
        self->completeLogin(result);
    }

private:
    bool ensureInitialized()
    {
        lastInitializeSendFailure_ = PushMessageSendResult{};
        lastInitializeLoginFailure_ = PushMessageLoginResult{};
        if (handle_) {
            return true;
        }
        if (config_.udpPassport.empty() || config_.tcpPassport.empty() || config_.appId <= 0) {
            lastInitializeSendFailure_ = sendFailure(
                -10524005,
                L"SdoBase initialize config is incomplete; check config.xml/clientinfo.xml or SDK AppID.");
            lastInitializeLoginFailure_ = loginFailure(
                -10524005,
                L"SdoBase initialize config is incomplete; check config.xml/clientinfo.xml or SDK AppID.");
            return false;
        }

        const int ret = SdoBase_Initialize3(
            config_.udpPassport.c_str(),
            config_.tcpPassport.c_str(),
            config_.appId,
            config_.areaId,
            config_.groupId,
            1,
            0,
            4,
            config_.productVersion.c_str(),
            2,
            config_.verifyCert,
            nullptr,
            onDynamicLoginCallback,
            nullptr,
            onLoginResultCallback,
            onGetDynamicKeyCallback,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            onSendPushMessageCallback,
            &handle_,
            config_.thirdLoginExtern,
            config_.channelId.c_str(),
            config_.gunionServerAddr1.c_str(),
            config_.gunionTcpServerAddr2.c_str(),
            config_.enableGunionPreInit);

        if (ret != 0 || !handle_) {
            const int mapped = mapLegacySdoBaseResultCode(ret);
            lastInitializeSendFailure_ = sendFailure(mapped, L"SdoBase_Initialize3 failed for push-message login.");
            lastInitializeLoginFailure_ = loginFailure(mapped, L"SdoBase_Initialize3 failed for push-message login.");
            return false;
        }

        SdoBase_SetUserData(handle_, this);
        SdoBase_SetGetDynamicKeyCallback(handle_, onGetDynamicKeyCallback);
        SdoBase_SetLoginResultCallback(handle_, onLoginResultCallback);
        SdoBase_SetSendPushMessageCallback(handle_, onSendPushMessageCallback);
        SdoBase_SetCallBack(handle_, SdoBase_CancelPushMessageCallback, onCancelPushMessageCallback);
        SdoBase_SetDynamicLoginCallback(handle_, onDynamicLoginCallback);
        SdoLsc_ProxyEnable(handle_, 0);
        logPushMessagePhase(L"SdoBase initialized and push-message callbacks registered");
        return true;
    }

    template <typename Fn>
    int callWithBusyRetry(Fn&& fn)
    {
        int error = 0;
        for (int attempt = 0; attempt < 3; ++attempt) {
            error = fn();
            if (error != ERROR_PROCESSING) {
                return error;
            }
            cancel();
            Sleep(300);
        }
        return error;
    }

    void handleDynamicKey(int resultCode, const char* failReason, const char* dynamicKey, const char* guid)
    {
        {
            std::wostringstream out;
            out << L"dynamic-key callback for push-message result=" << resultCode
                << L" dynamicKeyLen=" << (dynamicKey ? std::strlen(dynamicKey) : 0)
                << L" guidLen=" << (guid ? std::strlen(guid) : 0);
            logPushMessagePhase(out.str());
        }
        const int mapped = mapLegacySdoBaseResultCode(resultCode);
        if (mapped != 0) {
            completeSend(sendFailure(
                mapped,
                failReason && *failReason ? ansiToWideForLegacyAuth(failReason) : L"Get dynamic key failed for push-message login."));
            return;
        }
        if (!dynamicKey || !*dynamicKey) {
            completeSend(sendFailure(-10524100, L"Get dynamic key returned empty challenge for push-message login."));
            return;
        }

        SdoBase_SetUserData(handle_, this);
        const int cancelError = SdoBase_CancelPushMessageLogin(handle_);
        {
            std::wostringstream out;
            out << L"SdoBase_CancelPushMessageLogin returned " << cancelError;
            logPushMessagePhase(out.str());
        }
        if (cancelError != 0) {
            completeSend(sendFailure(
                mapLegacySdoBaseResultCode(cancelError),
                L"SdoBase_CancelPushMessageLogin call failed."));
            return;
        }
        if (shouldWaitForPushMessageCancelCallback(cancelError)) {
            logPushMessagePhase(L"waiting cancel push-message callback before SendPushMessage");
        }
    }

    void handleCancelPushMessage(int resultCode, const char* failReason)
    {
        const int mapped = mapLegacySdoBaseResultCode(resultCode);
        {
            std::wostringstream out;
            out << L"cancel push-message callback result=" << mapped;
            logPushMessagePhase(out.str());
        }
        if (!shouldContinuePushMessageAfterCancelCallback(mapped)) {
            completeSend(sendFailure(
                mapped,
                failReason && *failReason ? ansiToWideForLegacyAuth(failReason) : L"SdoBase_CancelPushMessageLogin callback failed."));
            return;
        }
        issueSendPushMessageAfterCancel(L"cancel-callback");
    }

    void issueSendPushMessageAfterCancel(const wchar_t* source)
    {
        if (!shouldIssuePushMessageSendAfterCancel(sendPushIssuedAfterCancel_)) {
            std::wostringstream skipped;
            skipped << L"SdoBase_SendPushMessage skipped because already issued source=" << source;
            logPushMessagePhase(skipped.str());
            return;
        }
        sendPushIssuedAfterCancel_ = true;
        SdoBase_SetUserData(handle_, this);
        const std::string scene = pushMessageLoginScene(isXpOrOlder());
        const int error = SdoBase_SendPushMessage(handle_, currentAccountAnsi_.c_str(), scene.c_str());
        {
            std::wostringstream out;
            out << L"SdoBase_SendPushMessage returned " << error
                << L" scene=" << ansiToWideForLegacyAuth(scene.c_str())
                << L" source=" << source;
            logPushMessagePhase(out.str());
        }
        if (error != 0) {
            completeSend(sendFailure(
                mapLegacySdoBaseResultCode(error),
                L"SdoBase_SendPushMessage call failed."));
        }
    }

    void handleSendPushMessage(int resultCode, const char* failReason, const char* pushMsgSerialNum)
    {
        const int mapped = mapLegacySdoBaseResultCode(resultCode);
        PushMessageSendResult result;
        result.success = mapped == 0;
        result.errorCode = mapped;
        result.errorMessage = failReason && *failReason ? ansiToWideForLegacyAuth(failReason) : std::wstring();
        result.serialNumber = pushMsgSerialNum ? pushMsgSerialNum : "";
        {
            std::wostringstream out;
            out << L"send push-message callback result=" << mapped
                << L" serialLen=" << result.serialNumber.size();
            logPushMessagePhase(out.str());
        }
        completeSend(result);
    }

    void handleLoginResult(int resultCode, const char* failReason, const char* sndaId, const char* ticket, const char* tgt, const char* inputUserId)
    {
        PushMessageLoginResult result = makePushMessageLoginResultFromLegacyCallback(
            resultCode,
            failReason,
            sndaId,
            ticket,
            tgt,
            inputUserId && *inputUserId ? inputUserId : currentAccountAnsi_.c_str());
        {
            std::wostringstream out;
            out << L"push-message login callback result=" << result.errorCode
                << L" success=" << result.success
                << L" continue=" << result.continuePolling
                << L" sndaIdLen=" << result.sndaId.size()
                << L" ticketLen=" << result.ticket.size()
                << L" tgtLen=" << result.sessionId.size();
            logPushMessagePhase(out.str());
        }
        completeLogin(result);
    }

#else
private:
#endif

    void completeSend(const PushMessageSendResult& result)
    {
        if (sendCompletion_) {
            auto completion = std::move(sendCompletion_);
            completion(result);
        }
    }

    void completeLogin(const PushMessageLoginResult& result)
    {
        if (loginCompletion_) {
            auto completion = std::move(loginCompletion_);
            completion(result);
        }
    }

    SdoBaseAuthConfig config_;
    std::wstring currentAccountWide_;
    std::string currentAccountAnsi_;
    PushMessageSendCompletion sendCompletion_;
    PushMessageLoginCompletion loginCompletion_;
    PushMessageSendResult lastInitializeSendFailure_;
    PushMessageLoginResult lastInitializeLoginFailure_;
    bool sendPushIssuedAfterCancel_ = false;
#if QTLOGIN_HAS_PUSH_MESSAGE_LEGACY_AUTH_LIBS
    SdoBaseHandle* handle_ = nullptr;
#endif
};

SdoBasePushMessageAuthenticator::SdoBasePushMessageAuthenticator(SdoBaseAuthConfig config)
    : impl_(std::make_unique<Impl>(std::move(config)))
{
}

SdoBasePushMessageAuthenticator::~SdoBasePushMessageAuthenticator() = default;

void SdoBasePushMessageAuthenticator::startLogin(const PushMessageLoginRequest& request, PushMessageSendCompletion completion)
{
    impl_->startLogin(request, std::move(completion));
}

void SdoBasePushMessageAuthenticator::pollLogin(PushMessageLoginCompletion completion)
{
    impl_->pollLogin(std::move(completion));
}

void SdoBasePushMessageAuthenticator::cancel()
{
    impl_->cancel();
}

}
