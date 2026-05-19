#include "sdo_base_password_auth.h"

#include <cstring>
#include <sstream>
#include <utility>

#include <windows.h>

#include "legacy_password_crypto.h"
#include "logger.h"

#if (defined(_M_IX86) || defined(__i386__)) && defined(QTLOGIN_ENABLE_LEGACY_AUTH_LIBS)
#define QTLOGIN_HAS_LEGACY_AUTH_LIBS 1
#include "SdoBaseClient.h"
#else
#define QTLOGIN_HAS_LEGACY_AUTH_LIBS 0
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

PasswordAuthResult failureResult(int errorCode, std::wstring message)
{
    PasswordAuthResult result;
    result.success = false;
    result.errorCode = errorCode;
    result.errorMessage = std::move(message);
    return result;
}

PasswordAuthResult unsupportedResult()
{
    return failureResult(-10524100, L"当前构建无法加载旧 32 位 SdoBaseClient/SafeStore；请切换到 VS2019 Win32 + 32 位 Qt 后再测试真实密码登录。");
}

void logPasswordPhase(const std::wstring& message)
{
    qtlogin::common::logLine(L"password-auth", message);
}

}

class SdoBasePasswordAuthenticator::Impl {
public:
    explicit Impl(SdoBaseAuthConfig config)
        : config_(std::move(config))
    {
#if QTLOGIN_HAS_LEGACY_AUTH_LIBS
        clientConfigEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        privacyConfigEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
#endif
    }

    ~Impl()
    {
#if QTLOGIN_HAS_LEGACY_AUTH_LIBS
        if (clientConfigOwner() == this) {
            clientConfigOwner() = nullptr;
        }
        if (privacyConfigOwner() == this) {
            privacyConfigOwner() = nullptr;
        }
        if (clientConfigEvent_) {
            CloseHandle(clientConfigEvent_);
            clientConfigEvent_ = nullptr;
        }
        if (privacyConfigEvent_) {
            CloseHandle(privacyConfigEvent_);
            privacyConfigEvent_ = nullptr;
        }
        if (handle_) {
            SdoBase_Release(handle_);
            handle_ = nullptr;
        }
#endif
    }

    void login(const PasswordLoginRequest& request, PasswordAuthCompletion completion)
    {
        completion_ = std::move(completion);
        const PasswordLoginValidationResult validation = validatePasswordLoginRequest(request);
        if (validation.code != PasswordLoginValidationCode::Ok) {
            complete(failureResult(-10524002, validation.message));
            return;
        }
        pendingRequest_ = request;
        pendingRequest_.account = validation.trimmedAccount;

#if QTLOGIN_HAS_LEGACY_AUTH_LIBS
        logPasswordPhase(L"password login begin: request dynamic key");
        if (!ensureInitialized()) {
            complete(lastInitializeFailure_);
            return;
        }
        SdoBase_SetUserData(handle_, this);
        const int error = SdoBase_GetDynamicKey(handle_);
        {
            std::wostringstream out;
            out << L"SdoBase_GetDynamicKey returned " << error;
            logPasswordPhase(out.str());
        }
        if (error != 0) {
            complete(failureResult(error, L"连接认证服务器失败，请稍后重试。"));
        }
#else
        complete(unsupportedResult());
#endif
    }

    StartupClientConfigResult fetchStartupClientConfig(int timeoutMs)
    {
        StartupClientConfigResult result;
        result.attempted = true;

#if QTLOGIN_HAS_LEGACY_AUTH_LIBS
        if (!ensureInitialized()) {
            result.errorCode = lastInitializeFailure_.errorCode;
            result.errorMessage = lastInitializeFailure_.errorMessage;
            return result;
        }
        if (!clientConfigEvent_) {
            result.errorCode = -10524012;
            result.errorMessage = L"getSystemConfig event is not initialized.";
            return result;
        }

        clientConfigResult_ = StartupClientConfigResult{};
        clientConfigResult_.attempted = true;
        ResetEvent(clientConfigEvent_);
        clientConfigOwner() = this;
        SdoBase_SetUserData(handle_, this);
        SdoBase_SetGetClientConfigCallback(handle_, onGetClientConfigCallback);

        logPasswordPhase(L"startup getSystemConfig begin");
        const int error = SdoBase_GetClientConfig(handle_);
        {
            std::wostringstream out;
            out << L"SdoBase_GetClientConfig returned " << error;
            logPasswordPhase(out.str());
        }
        if (error != 0) {
            result.errorCode = mapLegacySdoBaseResultCode(error);
            result.errorMessage = L"SdoBase_GetClientConfig call failed.";
            return result;
        }

        const DWORD waitMs = timeoutMs > 0 ? static_cast<DWORD>(timeoutMs) : 10000;
        const DWORD waitResult = WaitForSingleObject(clientConfigEvent_, waitMs);
        if (waitResult == WAIT_TIMEOUT) {
            result.timedOut = true;
            result.errorCode = -10524013;
            result.errorMessage = L"getSystemConfig timeout.";
            logPasswordPhase(L"startup getSystemConfig timeout");
            return result;
        }
        if (waitResult != WAIT_OBJECT_0) {
            result.errorCode = -10524015;
            result.errorMessage = L"getSystemConfig wait failed.";
            return result;
        }
        return clientConfigResult_;
#else
        result.errorCode = unsupportedResult().errorCode;
        result.errorMessage = unsupportedResult().errorMessage;
        return result;
#endif
    }

    UserPrivacyConfigResult fetchUserPrivacyConfig(
        int savedPrivacyPolicyVersion,
        int savedServiceAgreementVersion,
        int timeoutMs)
    {
        UserPrivacyConfigResult result;
        result.attempted = true;

#if QTLOGIN_HAS_LEGACY_AUTH_LIBS
        if (!ensureInitialized()) {
            result.errorCode = lastInitializeFailure_.errorCode;
            result.errorMessage = lastInitializeFailure_.errorMessage;
            return result;
        }
        if (!privacyConfigEvent_) {
            result.errorCode = -10524016;
            result.errorMessage = L"user privacy config event is not initialized.";
            return result;
        }

        privacyConfigResult_ = UserPrivacyConfigResult{};
        privacyConfigResult_.attempted = true;
        ResetEvent(privacyConfigEvent_);
        privacyConfigOwner() = this;
        SdoBase_SetUserData(handle_, this);
        SdoBase_SetUserPrivacyConfigCallback(handle_, onUserPrivacyConfigCallback);

        logPasswordPhase(L"user privacy config begin");
        const int error = SdoBase_UserPrivacyConfig(
            handle_,
            "optimisepc",
            savedPrivacyPolicyVersion,
            savedServiceAgreementVersion);
        {
            std::wostringstream out;
            out << L"SdoBase_UserPrivacyConfig returned " << error
                << L" savedPrivacy=" << savedPrivacyPolicyVersion
                << L" savedAgreement=" << savedServiceAgreementVersion;
            logPasswordPhase(out.str());
        }
        if (error != 0) {
            result.errorCode = mapLegacySdoBaseResultCode(error);
            result.errorMessage = L"SdoBase_UserPrivacyConfig call failed.";
            return result;
        }

        const DWORD waitMs = timeoutMs > 0 ? static_cast<DWORD>(timeoutMs) : 3000;
        const DWORD waitResult = WaitForSingleObject(privacyConfigEvent_, waitMs);
        if (waitResult == WAIT_TIMEOUT) {
            result.timedOut = true;
            result.errorCode = -10524017;
            result.errorMessage = L"user privacy config timeout.";
            logPasswordPhase(L"user privacy config timeout");
            return result;
        }
        if (waitResult != WAIT_OBJECT_0) {
            result.errorCode = -10524018;
            result.errorMessage = L"user privacy config wait failed.";
            return result;
        }
        return privacyConfigResult_;
#else
        result.errorCode = unsupportedResult().errorCode;
        result.errorMessage = unsupportedResult().errorMessage;
        return result;
#endif
    }

#if QTLOGIN_HAS_LEGACY_AUTH_LIBS
    static Impl*& clientConfigOwner()
    {
        static Impl* owner = nullptr;
        return owner;
    }

    static Impl*& privacyConfigOwner()
    {
        static Impl* owner = nullptr;
        return owner;
    }

    static void SDOAPI onGetClientConfigCallback(int resultCode, const char* failReason, const char* config)
    {
        if (Impl* self = clientConfigOwner()) {
            self->handleClientConfig(resultCode, failReason, config);
        }
    }

    static void SDOAPI onUserPrivacyConfigCallback(
        int resultCode,
        const char* failReason,
        const char* privacyPolicyUrl,
        int privacyPolicyVersion,
        const char* servicerAgreementUrl,
        int serviceAgreementVersion)
    {
        if (Impl* self = privacyConfigOwner()) {
            self->handleUserPrivacyConfig(
                resultCode,
                failReason,
                privacyPolicyUrl,
                privacyPolicyVersion,
                servicerAgreementUrl,
                serviceAgreementVersion);
        }
    }

    static void SDOAPI onGetDynamicKeyCallback(int resultCode, const char* failReason, const char* dynamicKey, const char* guid, SdoBaseHandle* handle)
    {
        auto* self = static_cast<Impl*>(SdoBase_GetUserData(handle));
        if (self) {
            self->handleDynamicKey(resultCode, failReason, dynamicKey, guid);
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
        {
            std::wostringstream out;
            out << L"dynamic-login callback result=" << resultCode
                << L" loginType=" << loginType
                << L" deviceType=" << deviceType;
            logPasswordPhase(out.str());
        }
        PasswordAuthResult result;
        result.success = false;
        result.needsSecondFactor = true;
        result.errorCode = mapLegacySdoBaseResultCode(resultCode);
        result.errorMessage = failReason && *failReason
            ? ansiToWideForLegacyAuth(failReason)
            : L"当前账号需要二次认证，Qt 重写版后续会继续接入图验/密保/实名流程。";
        self->complete(result);
    }

    static void SDOAPI onCheckCodeLoginCallback(int resultCode, const char* failReason, const char* checkCodeUrl,
        int isGeetestCode, int width, int height, SdoBaseHandle* handle)
    {
        (void)isGeetestCode;
        auto* self = static_cast<Impl*>(SdoBase_GetUserData(handle));
        if (!self) {
            return;
        }

        std::wostringstream out;
        out << L"check-code callback result=" << resultCode
            << L" urlLen=" << (checkCodeUrl ? std::strlen(checkCodeUrl) : 0)
            << L" size=" << width << L"x" << height;
        logPasswordPhase(out.str());

        PasswordAuthResult result;
        result.success = false;
        result.needsSecondFactor = resultCode == 0;
        result.needsCaptcha = resultCode == 0;
        result.errorCode = mapLegacySdoBaseResultCode(resultCode);
        result.errorMessage = failReason && *failReason
            ? ansiToWideForLegacyAuth(failReason)
            : L"Password login requires captcha verification.";
        result.challengeUrl = checkCodeUrl ? checkCodeUrl : "";
        result.challengeWidth = width;
        result.challengeHeight = height;
        self->complete(result);
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

private:
    bool ensureInitialized()
    {
        lastInitializeFailure_ = PasswordAuthResult{};
        if (handle_) {
            return true;
        }
        if (config_.udpPassport.empty() || config_.tcpPassport.empty() || config_.appId <= 0) {
            lastInitializeFailure_ = failureResult(-10524005, L"SdoBase initialize config is incomplete; check config.xml/clientinfo.xml or SDK AppID.");
            complete(failureResult(-10524005, L"SdoBase 初始化配置不完整，请检查 config.xml/clientinfo.xml 或 SDK 传入的 AppID。"));
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
            onCheckCodeLoginCallback,
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
            nullptr,
            &handle_,
            config_.thirdLoginExtern,
            config_.channelId.c_str(),
            config_.gunionServerAddr1.c_str(),
            config_.gunionTcpServerAddr2.c_str(),
            config_.enableGunionPreInit);

        if (ret != 0 || !handle_) {
            lastInitializeFailure_ = failureResult(mapLegacySdoBaseResultCode(ret), L"SdoBase_Initialize3 failed.");
            complete(failureResult(ret, L"SdoBase_Initialize3 初始化失败。"));
            return false;
        }
        SdoBase_SetCheckCodeLoginCallback(handle_, onCheckCodeLoginCallback);
        SdoBase_SetDynamicLoginCallback(handle_, onDynamicLoginCallback);
        SdoBase_SetLoginResultCallback(handle_, onLoginResultCallback);
        SdoBase_SetGetDynamicKeyCallback(handle_, onGetDynamicKeyCallback);
        SdoLsc_ProxyEnable(handle_, 0);
        logPasswordPhase(L"SdoBase initialized and password callbacks registered");
        return true;
    }

    void handleClientConfig(int resultCode, const char* failReason, const char* config)
    {
        clientConfigResult_.attempted = true;
        clientConfigResult_.errorCode = mapLegacySdoBaseResultCode(resultCode);
        if (resultCode != 0) {
            clientConfigResult_.success = false;
            clientConfigResult_.errorMessage = failReason && *failReason
                ? ansiToWideForLegacyAuth(failReason)
                : L"getSystemConfig failed.";
            logPasswordPhase(L"startup getSystemConfig callback failed");
            if (clientConfigEvent_) {
                SetEvent(clientConfigEvent_);
            }
            return;
        }

        clientConfigResult_.rawConfig = config && *config ? config : "{}";
        clientConfigResult_.config = qtlogin::config::parseClientConfigJson(clientConfigResult_.rawConfig);
        if (!clientConfigResult_.config.parseOk) {
            clientConfigResult_.success = false;
            clientConfigResult_.errorCode = -10524014;
            clientConfigResult_.errorMessage = L"getSystemConfig JSON parse failed.";
            logPasswordPhase(L"startup getSystemConfig JSON parse failed");
            if (clientConfigEvent_) {
                SetEvent(clientConfigEvent_);
            }
            return;
        }

        clientConfigResult_.success = true;
        {
            std::wostringstream out;
            out << L"startup getSystemConfig callback success rawLen="
                << clientConfigResult_.rawConfig.size()
                << L" loginMethodCount="
                << clientConfigResult_.config.loginMethodList.size();
            logPasswordPhase(out.str());
        }
        if (clientConfigEvent_) {
            SetEvent(clientConfigEvent_);
        }
    }

    void handleUserPrivacyConfig(
        int resultCode,
        const char* failReason,
        const char* privacyPolicyUrl,
        int privacyPolicyVersion,
        const char* servicerAgreementUrl,
        int serviceAgreementVersion)
    {
        privacyConfigResult_.attempted = true;
        privacyConfigResult_.errorCode = mapLegacySdoBaseResultCode(resultCode);
        privacyConfigResult_.privacyPolicyVersion = privacyPolicyVersion;
        privacyConfigResult_.serviceAgreementVersion = serviceAgreementVersion;
        privacyConfigResult_.privacyPolicyUrl = privacyPolicyUrl && *privacyPolicyUrl
            ? ansiToWideForLegacyAuth(privacyPolicyUrl)
            : std::wstring();
        privacyConfigResult_.serviceAgreementUrl = servicerAgreementUrl && *servicerAgreementUrl
            ? ansiToWideForLegacyAuth(servicerAgreementUrl)
            : std::wstring();

        if (resultCode != 0) {
            privacyConfigResult_.success = false;
            privacyConfigResult_.errorMessage = failReason && *failReason
                ? ansiToWideForLegacyAuth(failReason)
                : L"user privacy config failed.";
            logPasswordPhase(L"user privacy config callback failed");
            if (privacyConfigEvent_) {
                SetEvent(privacyConfigEvent_);
            }
            return;
        }

        privacyConfigResult_.success = true;
        {
            std::wostringstream out;
            out << L"user privacy config callback success"
                << L" privacyVersion=" << privacyConfigResult_.privacyPolicyVersion
                << L" serviceVersion=" << privacyConfigResult_.serviceAgreementVersion
                << L" privacyUrlLen=" << privacyConfigResult_.privacyPolicyUrl.size()
                << L" serviceUrlLen=" << privacyConfigResult_.serviceAgreementUrl.size();
            logPasswordPhase(out.str());
        }
        if (privacyConfigEvent_) {
            SetEvent(privacyConfigEvent_);
        }
    }

    void handleDynamicKey(int resultCode, const char* failReason, const char* dynamicKey, const char* guid)
    {
        {
            std::wostringstream out;
            out << L"dynamic-key callback result=" << resultCode
                << L" dynamicKeyLen=" << (dynamicKey ? std::strlen(dynamicKey) : 0)
                << L" guidLen=" << (guid ? std::strlen(guid) : 0);
            logPasswordPhase(out.str());
        }

        if (resultCode != 0) {
            complete(failureResult(
                mapLegacySdoBaseResultCode(resultCode),
                failReason && *failReason ? ansiToWideForLegacyAuth(failReason) : L"Get dynamic key failed."));
            return;
        }

        if (!dynamicKey || !*dynamicKey) {
            const bool guidReturned = guid && *guid;
            complete(failureResult(
                mapLegacySdoBaseResultCode(resultCode),
                guidReturned
                    ? L"GetGuid succeeded but dynamic key is empty; static password login cannot continue."
                    : L"Get dynamic key failed: empty dynamic key."));
            return;
        }

        if (resultCode != 0 || !dynamicKey || !*dynamicKey) {
            complete(failureResult(resultCode, failReason && *failReason ? ansiToWideForLegacyAuth(failReason) : L"获取动态密钥失败。"));
            return;
        }

        std::wstring errorMessage;
        std::string passwordTextBase64;
        const std::string passwordAnsi = wideToAnsiForLegacyAuth(pendingRequest_.password);
        if (!makeLegacyPasswordTextBase64(passwordAnsi, dynamicKey, &passwordTextBase64, &errorMessage)) {
            complete(failureResult(-10524100, errorMessage));
            return;
        }

        const LegacyStaticLoginPayload payload = makeLegacyStaticLoginPayload(
            pendingRequest_.account,
            passwordTextBase64,
            dynamicKey,
            pendingRequest_.inputUserType,
            pendingRequest_.keepLogin,
            [&errorMessage](const std::string& plain, const std::string& key) {
                std::string encrypted;
                if (!sdoaEncryptWithDynamicKey(plain, key, &encrypted, &errorMessage)) {
                    return std::string();
                }
                return encrypted;
            },
            isXpOrOlder());

        if (payload.inputUserId.empty() || payload.password.empty()) {
            complete(failureResult(-10524100, errorMessage.empty() ? L"账号或密码加密失败。" : errorMessage));
            return;
        }

        SdoBase_SetUserData(handle_, this);
        {
            std::wostringstream out;
            out << L"call SdoBase_StaticLoginWithGameAccount"
                << L" inputUserType=" << payload.inputUserType
                << L" keepLoginFlag=" << payload.keepLoginFlag
                << L" scene=" << ansiToWideForLegacyAuth(payload.scene);
            logPasswordPhase(out.str());
        }
        const int loginError = SdoBase_StaticLoginWithGameAccount(
            handle_,
            payload.inputUserId.c_str(),
            payload.password.c_str(),
            payload.accountDomain,
            payload.autoLoginFlag,
            payload.autoLoginKeepTime,
            payload.isSupportGeetest,
            payload.inputUserType,
            payload.keepLoginFlag,
            payload.scene.c_str());
        {
            std::wostringstream out;
            out << L"SdoBase_StaticLoginWithGameAccount returned " << loginError;
            logPasswordPhase(out.str());
        }
        if (loginError != 0) {
            complete(failureResult(
                mapLegacySdoBaseResultCode(loginError),
                L"SdoBase_StaticLoginWithGameAccount call failed."));
            return;
        }
        if (loginError != 0) {
            complete(failureResult(loginError, L"SdoBase_StaticLoginWithGameAccount 调用失败。"));
        }
    }

    void handleLoginResult(int resultCode, const char* failReason, const char* sndaId, const char* ticket, const char* tgt, const char* inputUserId)
    {
        {
            std::wostringstream out;
            out << L"login-result callback result=" << resultCode
                << L" sndaIdLen=" << (sndaId ? std::strlen(sndaId) : 0)
                << L" ticketLen=" << (ticket ? std::strlen(ticket) : 0)
                << L" tgtLen=" << (tgt ? std::strlen(tgt) : 0);
            logPasswordPhase(out.str());
        }
        PasswordAuthResult result;
        result.success = resultCode == 0;
        result.errorCode = mapLegacySdoBaseResultCode(resultCode);
        result.errorMessage = failReason && *failReason ? ansiToWideForLegacyAuth(failReason) : L"密码登录失败。";
        result.sndaId = sndaId ? sndaId : "";
        result.ticket = ticket ? ticket : "";
        result.sessionId = tgt ? tgt : "";
        result.inputUserId = inputUserId ? inputUserId : "";
        if (result.success) {
            result.errorMessage.clear();
        }
        complete(result);
    }

#else
private:
#endif

    void complete(const PasswordAuthResult& result)
    {
        if (completion_) {
            auto completion = std::move(completion_);
            completion(result);
        }
    }

    SdoBaseAuthConfig config_;
    PasswordLoginRequest pendingRequest_;
    PasswordAuthCompletion completion_;
    PasswordAuthResult lastInitializeFailure_;
#if QTLOGIN_HAS_LEGACY_AUTH_LIBS
    SdoBaseHandle* handle_ = nullptr;
    HANDLE clientConfigEvent_ = nullptr;
    HANDLE privacyConfigEvent_ = nullptr;
    StartupClientConfigResult clientConfigResult_;
    UserPrivacyConfigResult privacyConfigResult_;
#endif
};

SdoBasePasswordAuthenticator::SdoBasePasswordAuthenticator(SdoBaseAuthConfig config)
    : impl_(std::make_unique<Impl>(std::move(config)))
{
}

SdoBasePasswordAuthenticator::~SdoBasePasswordAuthenticator() = default;

StartupClientConfigResult SdoBasePasswordAuthenticator::fetchStartupClientConfig(int timeoutMs)
{
    return impl_->fetchStartupClientConfig(timeoutMs);
}

UserPrivacyConfigResult SdoBasePasswordAuthenticator::fetchUserPrivacyConfig(
    int savedPrivacyPolicyVersion,
    int savedServiceAgreementVersion,
    int timeoutMs)
{
    return impl_->fetchUserPrivacyConfig(savedPrivacyPolicyVersion, savedServiceAgreementVersion, timeoutMs);
}

void SdoBasePasswordAuthenticator::login(const PasswordLoginRequest& request, PasswordAuthCompletion completion)
{
    impl_->login(request, std::move(completion));
}

}
