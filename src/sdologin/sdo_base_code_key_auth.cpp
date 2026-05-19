#include "sdo_base_code_key_auth.h"

#include <cstring>
#include <sstream>
#include <utility>

#include <windows.h>

#include "legacy_password_auth.h"
#include "logger.h"

#if (defined(_M_IX86) || defined(__i386__)) && defined(QTLOGIN_ENABLE_LEGACY_AUTH_LIBS)
#define QTLOGIN_HAS_CODE_KEY_LEGACY_AUTH_LIBS 1
#include "SdoBaseClient.h"
#else
#define QTLOGIN_HAS_CODE_KEY_LEGACY_AUTH_LIBS 0
#endif

namespace qtlogin::sdologin {
namespace {

CodeKeyImageResult codeKeyImageFailure(int errorCode, std::wstring message)
{
    CodeKeyImageResult result;
    result.success = false;
    result.errorCode = errorCode;
    result.errorMessage = std::move(message);
    return result;
}

CodeKeyLoginResult codeKeyLoginFailure(int errorCode, std::wstring message)
{
    CodeKeyLoginResult result;
    result.success = false;
    result.errorCode = errorCode;
    result.errorMessage = std::move(message);
    return result;
}

CodeKeyImageResult unsupportedImageResult()
{
    return codeKeyImageFailure(-10524100, L"当前构建无法加载旧 32 位 SdoBaseClient，请使用 VS2019 Win32 + 32 位 Qt 构建测试真实扫码登录。");
}

CodeKeyLoginResult unsupportedLoginResult()
{
    return codeKeyLoginFailure(-10524100, L"当前构建无法加载旧 32 位 SdoBaseClient，请使用 VS2019 Win32 + 32 位 Qt 构建测试真实扫码登录。");
}

void logCodeKeyPhase(const std::wstring& message)
{
    qtlogin::common::logLine(L"code-key-auth", message);
}

}

class SdoBaseCodeKeyAuthenticator::Impl {
public:
    explicit Impl(SdoBaseAuthConfig config)
        : config_(std::move(config))
    {
    }

    ~Impl()
    {
#if QTLOGIN_HAS_CODE_KEY_LEGACY_AUTH_LIBS
        cancel();
        if (handle_) {
            SdoBase_Release(handle_);
            handle_ = nullptr;
        }
#endif
    }

    void requestCodeImage(int maxSize, CodeKeyImageCompletion completion)
    {
        imageCompletion_ = std::move(completion);
        maxSize_ = maxSize > 0 ? maxSize : 104;

#if QTLOGIN_HAS_CODE_KEY_LEGACY_AUTH_LIBS
        if (!ensureInitialized()) {
            completeImage(lastImageInitializeFailure_);
            return;
        }

        clearCodeKey();
        SdoBase_SetUserData(handle_, this);
        SdoBase_SetParam(handle_, "maxsize", std::to_string(maxSize_).c_str());
        logCodeKeyPhase(L"request code-key image begin: get dynamic key");

        const int error = callWithBusyRetry([this]() {
            return SdoBase_GetDynamicKey(handle_);
        });
        if (error != 0) {
            completeImage(codeKeyImageFailure(
                mapLegacySdoBaseResultCode(error),
                L"SdoBase_GetDynamicKey call failed for code-key login."));
        }
#else
        completeImage(unsupportedImageResult());
#endif
    }

    void pollLogin(int maxSize, CodeKeyLoginCompletion completion)
    {
        loginCompletion_ = std::move(completion);
        maxSize_ = maxSize > 0 ? maxSize : maxSize_;

#if QTLOGIN_HAS_CODE_KEY_LEGACY_AUTH_LIBS
        if (!ensureInitialized()) {
            completeLogin(lastLoginInitializeFailure_);
            return;
        }

        SdoBase_SetUserData(handle_, this);
        SdoBase_SetParam(handle_, "maxsize", std::to_string(maxSize_).c_str());
        const int error = SdoBase_QrCodeLogin(handle_, 0, 0, 0);
        {
            std::wostringstream out;
            out << L"SdoBase_QrCodeLogin returned " << error;
            logCodeKeyPhase(out.str());
        }
        if (error == ERROR_PROCESSING) {
            CodeKeyLoginResult result;
            result.continuePolling = true;
            result.errorCode = kCodeKeyContinuePollingErrorCode;
            completeLogin(result);
            return;
        }
        if (error != 0) {
            completeLogin(codeKeyLoginFailure(
                mapLegacySdoBaseResultCode(error),
                L"SdoBase_QrCodeLogin call failed."));
        }
#else
        completeLogin(unsupportedLoginResult());
#endif
    }

    void clearCodeKey()
    {
#if QTLOGIN_HAS_CODE_KEY_LEGACY_AUTH_LIBS
        if (handle_) {
            SdoBase_ClearCodeKey(handle_);
        }
#endif
    }

    void cancel()
    {
#if QTLOGIN_HAS_CODE_KEY_LEGACY_AUTH_LIBS
        if (handle_) {
            SdoBase_Cancel(handle_);
        }
#endif
    }

#if QTLOGIN_HAS_CODE_KEY_LEGACY_AUTH_LIBS
    static void SDOAPI onGetDynamicKeyCallback(int resultCode, const char* failReason, const char* dynamicKey, const char* guid, SdoBaseHandle* handle)
    {
        (void)dynamicKey;
        (void)guid;
        auto* self = static_cast<Impl*>(SdoBase_GetUserData(handle));
        if (self) {
            self->handleDynamicKey(resultCode, failReason);
        }
    }

    static void SDOAPI onGetQrCodeCallback(int resultCode, const char* failReason, const char* picData, int picLength, SdoBaseHandle* handle)
    {
        auto* self = static_cast<Impl*>(SdoBase_GetUserData(handle));
        if (self) {
            self->handleQrCode(resultCode, failReason, picData, picLength);
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
        auto* self = static_cast<Impl*>(SdoBase_GetUserData(handle));
        if (self) {
            self->handleLoginResult(resultCode, failReason, sndaId, ticket, tgt, inputUserId, isScanned);
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
        CodeKeyLoginResult result;
        result.success = false;
        result.needsSecondFactor = true;
        result.errorCode = mapLegacySdoBaseResultCode(resultCode);
        result.errorMessage = failReason && *failReason
            ? ansiToWideForLegacyAuth(failReason)
            : L"扫码登录需要继续二次认证，Qt 重写版后续会接入对应弹框。";
        self->completeLogin(result);
    }

private:
    bool ensureInitialized()
    {
        lastImageInitializeFailure_ = CodeKeyImageResult{};
        lastLoginInitializeFailure_ = CodeKeyLoginResult{};
        if (handle_) {
            return true;
        }
        if (config_.udpPassport.empty() || config_.tcpPassport.empty() || config_.appId <= 0) {
            lastImageInitializeFailure_ = codeKeyImageFailure(
                -10524005,
                L"SdoBase initialize config is incomplete; check config.xml/clientinfo.xml or SDK AppID.");
            lastLoginInitializeFailure_ = codeKeyLoginFailure(
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
            onGetQrCodeCallback,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            &handle_,
            config_.thirdLoginExtern,
            config_.channelId.c_str(),
            config_.gunionServerAddr1.c_str(),
            config_.gunionTcpServerAddr2.c_str(),
            config_.enableGunionPreInit);

        if (ret != 0 || !handle_) {
            const int mapped = mapLegacySdoBaseResultCode(ret);
            lastImageInitializeFailure_ = codeKeyImageFailure(mapped, L"SdoBase_Initialize3 failed for code-key login.");
            lastLoginInitializeFailure_ = codeKeyLoginFailure(mapped, L"SdoBase_Initialize3 failed for code-key login.");
            return false;
        }

        SdoBase_SetUserData(handle_, this);
        SdoBase_SetGetDynamicKeyCallback(handle_, onGetDynamicKeyCallback);
        SdoBase_SetGetQrCodeCallback(handle_, onGetQrCodeCallback);
        SdoBase_SetLoginResultCallback(handle_, onLoginResultCallback);
        SdoBase_SetDynamicLoginCallback(handle_, onDynamicLoginCallback);
        SdoLsc_ProxyEnable(handle_, 0);
        logCodeKeyPhase(L"SdoBase initialized and code-key callbacks registered");
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

    void handleDynamicKey(int resultCode, const char* failReason)
    {
        {
            std::wostringstream out;
            out << L"dynamic-key callback for code-key result=" << resultCode;
            logCodeKeyPhase(out.str());
        }
        const int mapped = mapLegacySdoBaseResultCode(resultCode);
        if (mapped != 0) {
            completeImage(codeKeyImageFailure(
                mapped,
                failReason && *failReason ? ansiToWideForLegacyAuth(failReason) : L"Get dynamic key failed for code-key login."));
            return;
        }

        SdoBase_SetUserData(handle_, this);
        SdoBase_SetParam(handle_, "maxsize", std::to_string(maxSize_).c_str());
        const int error = callWithBusyRetry([this]() {
            return SdoBase_GetQrCode(handle_);
        });
        {
            std::wostringstream out;
            out << L"SdoBase_GetQrCode returned " << error << L" maxSize=" << maxSize_;
            logCodeKeyPhase(out.str());
        }
        if (error != 0) {
            completeImage(codeKeyImageFailure(
                mapLegacySdoBaseResultCode(error),
                L"SdoBase_GetQrCode call failed."));
        }
    }

    void handleQrCode(int resultCode, const char* failReason, const char* picData, int picLength)
    {
        {
            std::wostringstream out;
            out << L"get-code-key callback result=" << resultCode << L" picLength=" << picLength;
            logCodeKeyPhase(out.str());
        }
        const int mapped = mapLegacySdoBaseResultCode(resultCode);
        if (mapped != 0) {
            completeImage(codeKeyImageFailure(
                mapped,
                failReason && *failReason ? ansiToWideForLegacyAuth(failReason) : L"Get QR code image failed."));
            return;
        }
        if (!picData || picLength <= 0) {
            completeImage(codeKeyImageFailure(-10524021, L"Get QR code image returned empty image data."));
            return;
        }

        CodeKeyImageResult result;
        result.success = true;
        result.errorCode = 0;
        result.imageBytes.assign(
            reinterpret_cast<const unsigned char*>(picData),
            reinterpret_cast<const unsigned char*>(picData) + picLength);
        completeImage(result);
    }

    void handleLoginResult(int resultCode, const char* failReason, const char* sndaId, const char* ticket, const char* tgt, const char* inputUserId, const char* isScanned)
    {
        CodeKeyLoginResult result = makeCodeKeyLoginResultFromLegacyCallback(
            resultCode,
            failReason,
            sndaId,
            ticket,
            tgt,
            inputUserId,
            isScanned);
        {
            std::wostringstream out;
            out << L"code-key login callback result=" << result.errorCode
                << L" success=" << result.success
                << L" scanned=" << result.scanned
                << L" continue=" << result.continuePolling
                << L" sndaIdLen=" << result.sndaId.size()
                << L" ticketLen=" << result.ticket.size()
                << L" tgtLen=" << result.sessionId.size();
            logCodeKeyPhase(out.str());
        }
        completeLogin(result);
    }

#else
private:
#endif

    void completeImage(const CodeKeyImageResult& result)
    {
        if (imageCompletion_) {
            auto completion = std::move(imageCompletion_);
            completion(result);
        }
    }

    void completeLogin(const CodeKeyLoginResult& result)
    {
        if (loginCompletion_) {
            auto completion = std::move(loginCompletion_);
            completion(result);
        }
    }

    SdoBaseAuthConfig config_;
    int maxSize_ = 104;
    CodeKeyImageCompletion imageCompletion_;
    CodeKeyLoginCompletion loginCompletion_;
    CodeKeyImageResult lastImageInitializeFailure_;
    CodeKeyLoginResult lastLoginInitializeFailure_;
#if QTLOGIN_HAS_CODE_KEY_LEGACY_AUTH_LIBS
    SdoBaseHandle* handle_ = nullptr;
#endif
};

SdoBaseCodeKeyAuthenticator::SdoBaseCodeKeyAuthenticator(SdoBaseAuthConfig config)
    : impl_(std::make_unique<Impl>(std::move(config)))
{
}

SdoBaseCodeKeyAuthenticator::~SdoBaseCodeKeyAuthenticator() = default;

void SdoBaseCodeKeyAuthenticator::requestCodeImage(int maxSize, CodeKeyImageCompletion completion)
{
    impl_->requestCodeImage(maxSize, std::move(completion));
}

void SdoBaseCodeKeyAuthenticator::pollLogin(int maxSize, CodeKeyLoginCompletion completion)
{
    impl_->pollLogin(maxSize, std::move(completion));
}

void SdoBaseCodeKeyAuthenticator::clearCodeKey()
{
    impl_->clearCodeKey();
}

void SdoBaseCodeKeyAuthenticator::cancel()
{
    impl_->cancel();
}

}
