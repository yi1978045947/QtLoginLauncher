#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#include <windows.h>

#include "client_config_model.h"
#include "config_manager.h"
#include "process_util.h"

namespace {

void writeTextFile(const std::wstring& path, const char* text)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    assert(file != INVALID_HANDLE_VALUE);
    DWORD written = 0;
    const DWORD size = static_cast<DWORD>(std::strlen(text));
    assert(WriteFile(file, text, size, &written, nullptr));
    assert(written == size);
    CloseHandle(file);
}

std::string readTextFile(const std::wstring& path)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    assert(file != INVALID_HANDLE_VALUE);
    LARGE_INTEGER size{};
    assert(GetFileSizeEx(file, &size));
    std::string text(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    assert(text.empty() || ReadFile(file, text.data(), static_cast<DWORD>(text.size()), &read, nullptr));
    assert(read == text.size());
    CloseHandle(file);
    return text;
}

size_t countOccurrences(const std::string& text, const std::string& needle)
{
    size_t count = 0;
    size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

bool directoryExists(const std::wstring& path)
{
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring defaultConfigRoot()
{
    if (directoryExists(L"assets\\default_config")) {
        return L"assets\\default_config";
    }
    return L"qtlogin_rewrite\\assets\\default_config";
}

}

int main()
{
    wchar_t tempPath[MAX_PATH] = {};
    assert(GetTempPathW(MAX_PATH, tempPath) > 0);

    const std::wstring root = qtlogin::common::joinPath(tempPath, L"qtlogin_config_tests");
    const std::wstring configDir = qtlogin::common::joinPath(root, L"config");
    CreateDirectoryW(root.c_str(), nullptr);
    CreateDirectoryW(configDir.c_str(), nullptr);

    writeTextFile(qtlogin::common::joinPath(configDir, L"clientinfo.xml"),
        "<?xml version=\"1.0\" encoding=\"utf-8\"?><ClientInfo>"
        "<EnableQuickLogin value=\"False\" />"
        "<SkinType value=\"4\" />"
        "<AppName value=\"QtLogin\" />"
        "</ClientInfo>");
    writeTextFile(qtlogin::common::joinPath(configDir, L"config.xml"),
        "<Config><TcpPassport value=\"http://cas.sdo.com\" /><ProtocolUrl value=\"https://example.test/protocol\" />"
        "<MaxAccountCount value=\"3\" /></Config>");
    writeTextFile(qtlogin::common::joinPath(configDir, L"version.xml"),
        "<VersionInfo><Version value=\"1.2.3\" /></VersionInfo>");
    const std::wstring userInfoPath = qtlogin::common::joinPath(configDir, L"userinfo.xml");
    writeTextFile(userInfoPath,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?><UserInfo>"
        "<Alert value=\"True\" />"
        "<Accounts>"
        "<Account name=\"oldAlpha\" type=\"0\" />"
        "<Account name=\"oldBeta\" type=\"1\" />"
        "</Accounts>"
        "</UserInfo>");

    qtlogin::config::ConfigManager config;
    assert(config.load(root));
    assert(config.clientInfoValue(L"SkinType") == L"4");
    assert(config.clientInfoValue(L"AppName") == L"QtLogin");
    assert(config.systemValue(L"TcpPassport") == L"http://cas.sdo.com");
    assert(config.maxAccountCount(10) == 3);
    assert(config.value(qtlogin::config::ConfigFile::Version, L"Version") == L"1.2.3");
    assert(!config.quickLoginEnabled());
    assert(config.integerValue(qtlogin::config::ConfigFile::UserInfo, L"PrivacyPolicyVersion", -1) == -1);
    assert(config.setIntegerValue(qtlogin::config::ConfigFile::UserInfo, L"PrivacyPolicyVersion", 7));
    qtlogin::config::ConfigManager updatedConfig;
    assert(updatedConfig.load(root));
    assert(updatedConfig.integerValue(qtlogin::config::ConfigFile::UserInfo, L"PrivacyPolicyVersion", -1) == 7);
    assert(updatedConfig.setIntegerValue(qtlogin::config::ConfigFile::UserInfo, L"PrivacyPolicyVersion", 8));
    const std::string updatedUserInfo = readTextFile(userInfoPath);
    assert(updatedUserInfo.find("<Account name=\"oldAlpha\" type=\"0\" />") != std::string::npos);
    assert(updatedUserInfo.find("<PrivacyPolicyVersion value=\"8\" />") != std::string::npos);
    assert(countOccurrences(updatedUserInfo, "PrivacyPolicyVersion") == 1);

    const auto initialAccounts = updatedConfig.userAccounts(updatedConfig.maxAccountCount(10));
    assert(initialAccounts.size() == 2);
    assert(initialAccounts[0].name == L"oldAlpha");
    assert(initialAccounts[0].type == 0);
    assert(initialAccounts[1].name == L"oldBeta");
    assert(initialAccounts[1].type == 1);

    assert(updatedConfig.recordUserAccount(L"  oldBeta  ", 0, updatedConfig.maxAccountCount(10)));
    assert(updatedConfig.recordUserAccount(L"gamma", 1, updatedConfig.maxAccountCount(10)));
    assert(updatedConfig.recordUserAccount(L"delta", 0, updatedConfig.maxAccountCount(10)));
    assert(updatedConfig.recordUserAccount(L"epsilon", 0, updatedConfig.maxAccountCount(10)));
    qtlogin::config::ConfigManager accountReload;
    assert(accountReload.load(root));
    const auto recordedAccounts = accountReload.userAccounts(accountReload.maxAccountCount(10));
    assert(recordedAccounts.size() == 3);
    assert(recordedAccounts[0].name == L"epsilon");
    assert(recordedAccounts[1].name == L"delta");
    assert(recordedAccounts[2].name == L"gamma");
    assert(recordedAccounts[2].type == 1);
    assert(accountReload.removeUserAccount(L"DELTA"));
    qtlogin::config::ConfigManager accountRemovedReload;
    assert(accountRemovedReload.load(root));
    const auto removedAccounts = accountRemovedReload.userAccounts(accountRemovedReload.maxAccountCount(10));
    assert(removedAccounts.size() == 2);
    assert(removedAccounts[0].name == L"epsilon");
    assert(removedAccounts[1].name == L"gamma");

    qtlogin::config::ConfigManager defaultConfig;
    assert(defaultConfig.load(defaultConfigRoot()));
    assert(defaultConfig.systemValue(L"PrivacyPolicyUrl") == L"https://we.sdoprofile.com/common/static/register/public/privacy_game.html");
    assert(defaultConfig.systemValue(L"ServicerAgreementUrl") == L"https://we.sdoprofile.com/common/static/register/public/user_agreements.html");
    assert(defaultConfig.systemValue(L"SupplementaryRulesUrl") == L"https://mxd.web.sdo.com/web7/news/violation.html");
    assert(defaultConfig.systemValue(L"DaoyuHomeUrl") == L"https://www.daoyu8.com/#/");
    assert(defaultConfig.systemValue(L"ForgotPasswordUrl") == L"https://i.sdo.com/index/findPassword");
    assert(defaultConfig.maxAccountCount(10) == 10);

    const std::string serverJson =
        "{"
        "\"voiceSendTip\":\"voice-send\","
        "\"voiceRemindTip\":\"voice-remind\","
        "\"supportMobileAccountOnly\":1,"
        "\"supportMobileAccountOnlyTip\":\"mobile-only-tip\","
        "\"forbiddenStaticPwd\":1,"
        "\"forbiddenStaticPwdTip\":\"dynamic-password-tip\","
        "\"realNameNeedTwiceConfirm\":1,"
        "\"realNameTwiceConfirmPrompt\":\"realname-confirm\","
        "\"activationCodeHint\":\"{\\\"activationCodeTitle\\\":\\\"activation-title\\\",\\\"activationCodeTips\\\":\\\"activation-tips\\\"}\","
        "\"loginMethodList\":[\"pwdLogin\",\"pushMessageLogin\",\"codeKeyLogin\",\"safePhoneSmsLogin\"]"
        "}";
    const qtlogin::config::ClientConfigModel clientConfig =
        qtlogin::config::parseClientConfigJson(serverJson);
    assert(clientConfig.voiceSendTip == "voice-send");
    assert(clientConfig.voiceRemindTip == "voice-remind");
    assert(clientConfig.supportMobileAccountOnly);
    assert(clientConfig.supportMobileAccountOnlyTip == "mobile-only-tip");
    assert(clientConfig.forbiddenStaticPwd);
    assert(clientConfig.forbiddenStaticPwdTip == "dynamic-password-tip");
    assert(clientConfig.realNameNeedTwiceConfirm == "1");
    assert(clientConfig.realNameTwiceConfirmPrompt == "realname-confirm");
    assert(clientConfig.activationCodeTitle == "activation-title");
    assert(clientConfig.activationCodeTips == "activation-tips");
    assert(clientConfig.loginMethodList.size() == 4);
    assert(clientConfig.loginMethodList[0] == qtlogin::config::LoginMethod::Password);
    assert(clientConfig.loginMethodList[1] == qtlogin::config::LoginMethod::PushMessage);
    assert(clientConfig.loginMethodList[2] == qtlogin::config::LoginMethod::QrCode);
    assert(clientConfig.loginMethodList[3] == qtlogin::config::LoginMethod::SafePhoneSms);
    assert(qtlogin::config::parseClientConfigJson("{bad json").parseOk == false);

    std::cout << "config_tests passed\n";
    return 0;
}
