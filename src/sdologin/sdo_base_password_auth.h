#pragma once

#include <functional>
#include <memory>
#include <string>

#include "client_config_model.h"
#include "legacy_password_auth.h"

namespace qtlogin::sdologin {

struct SdoBaseAuthConfig {
    std::string udpPassport;
    std::string tcpPassport;
    std::string productVersion = "1.0.0";
    int appId = 0;
    int areaId = 0;
    int groupId = 0;
    int thirdLoginExtern = 0;
    bool verifyCert = false;
    std::string channelId;
    std::string gunionServerAddr1;
    std::string gunionTcpServerAddr2;
    bool enableGunionPreInit = false;
};

struct PasswordAuthResult {
    bool success = false;
    bool needsSecondFactor = false;
    bool needsCaptcha = false;
    int errorCode = 0;
    std::wstring errorMessage;
    std::string challengeUrl;
    int challengeWidth = 0;
    int challengeHeight = 0;
    std::string sndaId;
    std::string ticket;
    std::string sessionId;
    std::string inputUserId;
};

struct StartupClientConfigResult {
    bool attempted = false;
    bool success = false;
    bool timedOut = false;
    int errorCode = 0;
    std::wstring errorMessage;
    std::string rawConfig;
    qtlogin::config::ClientConfigModel config;
};

struct UserPrivacyConfigResult {
    bool attempted = false;
    bool success = false;
    bool timedOut = false;
    int errorCode = 0;
    std::wstring errorMessage;
    std::wstring privacyPolicyUrl;
    int privacyPolicyVersion = -1;
    std::wstring serviceAgreementUrl;
    int serviceAgreementVersion = -1;
};

using PasswordAuthCompletion = std::function<void(const PasswordAuthResult& result)>;

class SdoBasePasswordAuthenticator final {
public:
    explicit SdoBasePasswordAuthenticator(SdoBaseAuthConfig config);
    ~SdoBasePasswordAuthenticator();

    SdoBasePasswordAuthenticator(const SdoBasePasswordAuthenticator&) = delete;
    SdoBasePasswordAuthenticator& operator=(const SdoBasePasswordAuthenticator&) = delete;

    StartupClientConfigResult fetchStartupClientConfig(int timeoutMs);
    UserPrivacyConfigResult fetchUserPrivacyConfig(
        int savedPrivacyPolicyVersion,
        int savedServiceAgreementVersion,
        int timeoutMs);
    void login(const PasswordLoginRequest& request, PasswordAuthCompletion completion);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
