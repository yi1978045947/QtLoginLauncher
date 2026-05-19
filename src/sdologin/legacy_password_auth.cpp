#include "legacy_password_auth.h"

#include <algorithm>
#include <array>

#include <windows.h>

namespace qtlogin::sdologin {
namespace {

constexpr char kBase64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base64Index(char ch)
{
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A';
    }
    if (ch >= 'a' && ch <= 'z') {
        return ch - 'a' + 26;
    }
    if (ch >= '0' && ch <= '9') {
        return ch - '0' + 52;
    }
    if (ch == '+') {
        return 62;
    }
    if (ch == '/') {
        return 63;
    }
    return -1;
}

}

std::wstring trimLegacyAccount(const std::wstring& account)
{
    const std::wstring delimiters = L" \r\n\t";
    const size_t begin = account.find_first_not_of(delimiters);
    if (begin == std::wstring::npos) {
        return L"";
    }
    const size_t end = account.find_last_not_of(delimiters);
    return account.substr(begin, end - begin + 1);
}

std::string wideToAnsiForLegacyAuth(const std::wstring& text)
{
    if (text.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string output(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), output.data(), size, nullptr, nullptr);
    return output;
}

std::wstring ansiToWideForLegacyAuth(const std::string& text)
{
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring output(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), output.data(), size);
    return output;
}

PasswordLoginValidationResult validatePasswordLoginRequest(const PasswordLoginRequest& request)
{
    PasswordLoginValidationResult result;
    result.trimmedAccount = trimLegacyAccount(request.account);
    if (!request.protocolAccepted) {
        result.code = PasswordLoginValidationCode::ProtocolRequired;
        result.message = L"请您详细阅读协议，并勾选同意!";
        return result;
    }
    if (result.trimmedAccount.empty()) {
        result.code = PasswordLoginValidationCode::EmptyAccount;
        result.message = L"请输入盛趣游戏通行证账号！";
        return result;
    }
    if (request.password.empty()) {
        result.code = PasswordLoginValidationCode::EmptyPassword;
        result.message = L"请输入盛趣游戏通行证密码！";
        return result;
    }
    result.code = PasswordLoginValidationCode::Ok;
    return result;
}

std::string base64EncodeForLegacyAuth(const std::string& source)
{
    std::string output;
    output.reserve(((source.size() + 2) / 3) * 4);
    for (size_t i = 0; i < source.size();) {
        const unsigned char octetA = static_cast<unsigned char>(source[i++]);
        const bool hasB = i < source.size();
        const unsigned char octetB = hasB ? static_cast<unsigned char>(source[i++]) : 0;
        const bool hasC = i < source.size();
        const unsigned char octetC = hasC ? static_cast<unsigned char>(source[i++]) : 0;
        const uint32_t triple = (static_cast<uint32_t>(octetA) << 16) |
            (static_cast<uint32_t>(octetB) << 8) |
            static_cast<uint32_t>(octetC);
        output.push_back(kBase64Table[(triple >> 18) & 0x3f]);
        output.push_back(kBase64Table[(triple >> 12) & 0x3f]);
        output.push_back(hasB ? kBase64Table[(triple >> 6) & 0x3f] : '=');
        output.push_back(hasC ? kBase64Table[triple & 0x3f] : '=');
    }
    return output;
}

std::string base64DecodeForLegacyAuth(const std::string& source)
{
    std::string output;
    output.reserve((source.size() / 4) * 3);
    for (size_t i = 0; i < source.size();) {
        std::array<int, 4> value = {0, 0, 0, 0};
        std::array<bool, 4> padded = {false, false, false, false};
        for (int part = 0; part < 4 && i < source.size(); ++part, ++i) {
            if (source[i] == '=') {
                padded[part] = true;
                value[part] = 0;
            } else {
                value[part] = base64Index(source[i]);
                if (value[part] < 0) {
                    return {};
                }
            }
        }
        const uint32_t triple = (static_cast<uint32_t>(value[0]) << 18) |
            (static_cast<uint32_t>(value[1]) << 12) |
            (static_cast<uint32_t>(value[2]) << 6) |
            static_cast<uint32_t>(value[3]);
        output.push_back(static_cast<char>((triple >> 16) & 0xff));
        if (!padded[2]) {
            output.push_back(static_cast<char>((triple >> 8) & 0xff));
        }
        if (!padded[3]) {
            output.push_back(static_cast<char>(triple & 0xff));
        }
    }
    return output;
}

std::string passwordLoginScene(bool isXpOrOlder)
{
    return isXpOrOlder ? "xp_login" : "pc_login";
}

int mapLegacySdoBaseResultCode(int resultCode)
{
    if (resultCode >= -10130200 && resultCode <= -10130100) {
        return resultCode - 394000;
    }
    return resultCode;
}

LegacyStaticLoginPayload makeLegacyStaticLoginPayload(
    const std::wstring& account,
    const std::string& rsaPasswordBase64,
    const std::string& dynamicKey,
    int inputUserType,
    bool keepLogin,
    const LegacySdoaEncryptor& encryptor,
    bool isXpOrOlder)
{
    LegacyStaticLoginPayload payload;
    const std::string accountAnsi = wideToAnsiForLegacyAuth(trimLegacyAccount(account));
    const std::string passwordBytes = base64DecodeForLegacyAuth(rsaPasswordBase64);
    payload.inputUserId = encryptor ? encryptor(accountAnsi, dynamicKey) : std::string();
    payload.password = encryptor ? encryptor(passwordBytes, dynamicKey) : std::string();
    payload.inputUserType = inputUserType;
    payload.keepLoginFlag = keepLogin ? 1 : 0;
    payload.scene = passwordLoginScene(isXpOrOlder);
    return payload;
}

}
