#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace qtlogin::sdologin {

enum class PasswordLoginValidationCode {
    Ok,
    ProtocolRequired,
    EmptyAccount,
    EmptyPassword,
};

struct PasswordLoginRequest {
    std::wstring account;
    std::wstring password;
    bool protocolAccepted = false;
    int inputUserType = 0;
    bool keepLogin = false;
};

struct PasswordLoginValidationResult {
    PasswordLoginValidationCode code = PasswordLoginValidationCode::Ok;
    std::wstring message;
    std::wstring trimmedAccount;
};

struct LegacyStaticLoginPayload {
    std::string inputUserId;
    std::string password;
    int accountDomain = 1;
    int autoLoginFlag = 0;
    int autoLoginKeepTime = 0;
    int isSupportGeetest = 2;
    int inputUserType = 0;
    int keepLoginFlag = 0;
    std::string scene;
};

using LegacySdoaEncryptor = std::function<std::string(const std::string& plain, const std::string& dynamicKey)>;

std::wstring trimLegacyAccount(const std::wstring& account);
std::string wideToAnsiForLegacyAuth(const std::wstring& text);
std::wstring ansiToWideForLegacyAuth(const std::string& text);

PasswordLoginValidationResult validatePasswordLoginRequest(const PasswordLoginRequest& request);

std::string base64EncodeForLegacyAuth(const std::string& source);
std::string base64DecodeForLegacyAuth(const std::string& source);
std::string passwordLoginScene(bool isXpOrOlder);
int mapLegacySdoBaseResultCode(int resultCode);

LegacyStaticLoginPayload makeLegacyStaticLoginPayload(
    const std::wstring& account,
    const std::string& rsaPasswordBase64,
    const std::string& dynamicKey,
    int inputUserType,
    bool keepLogin,
    const LegacySdoaEncryptor& encryptor,
    bool isXpOrOlder = false);

}
