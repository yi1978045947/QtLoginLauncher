#pragma once

#include <string>
#include <vector>

namespace qtlogin::config {

enum class LoginMethod {
    Password = 0,
    PushMessage = 4,
    QrCode = 2,
    SafePhoneSms = 9,
};

struct ClientConfigModel {
    bool parseOk = true;
    std::string voiceSendTip;
    std::string voiceRemindTip;
    bool supportMobileAccountOnly = false;
    std::string supportMobileAccountOnlyTip;
    bool forbiddenStaticPwd = false;
    std::string forbiddenStaticPwdTip;
    std::string realNameNeedTwiceConfirm = "0";
    std::string realNameTwiceConfirmPrompt;
    std::string activationCodeTitle;
    std::string activationCodeTips;
    std::vector<LoginMethod> loginMethodList;
};

ClientConfigModel parseClientConfigJson(const std::string& json);
bool hasLoginMethod(const ClientConfigModel& config, LoginMethod method);

}
