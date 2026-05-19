#include "push_message_login_model.h"

#include "legacy_password_auth.h"

namespace qtlogin::sdologin {
namespace {

std::wstring trimWhitespace(const std::wstring& value)
{
    const wchar_t* whitespace = L" \t\r\n";
    const std::wstring::size_type first = value.find_first_not_of(whitespace);
    if (first == std::wstring::npos) {
        return std::wstring();
    }
    const std::wstring::size_type last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

bool isMobileAccountText(const std::wstring& account)
{
    if (account.empty()) {
        return false;
    }
    for (wchar_t ch : account) {
        if ((ch < L'0' || ch > L'9') && ch != L'+') {
            return false;
        }
    }
    return true;
}

}

PushMessageLoginValidationResult validatePushMessageLoginRequest(const PushMessageLoginRequest& request)
{
    PushMessageLoginValidationResult result;
    result.trimmedAccount = trimWhitespace(request.account);

    if (!request.protocolAccepted) {
        result.code = PushMessageLoginValidationCode::ProtocolRequired;
        result.message = L"请您详细阅读协议，并勾选同意!";
        return result;
    }

    if (result.trimmedAccount.empty()) {
        result.code = PushMessageLoginValidationCode::EmptyAccount;
        result.message = request.supportMobileAccountOnly
            ? L"请输入盛趣游戏通行证手机号"
            : L"您还没有填写登录账号,请填写登录账号后重试";
        return result;
    }

    if (request.supportMobileAccountOnly && !isMobileAccountText(result.trimmedAccount)) {
        result.code = PushMessageLoginValidationCode::InvalidMobileAccount;
        result.message = L"输入账号格式错误，请重新输入";
        return result;
    }

    result.code = PushMessageLoginValidationCode::Ok;
    return result;
}

std::string pushMessageLoginScene(bool xpOrOlder)
{
    return xpOrOlder ? "xp_pushmsglogin" : "pc_pushmsglogin";
}

int normalizedPushMessageRetryCountdownSeconds(int configuredSeconds)
{
    return configuredSeconds > 0 ? configuredSeconds : 6;
}

bool shouldIssuePushMessageSendAfterCancel(bool sendAlreadyIssued)
{
    return !sendAlreadyIssued;
}

bool shouldWaitForPushMessageCancelCallback(int cancelCallError)
{
    return cancelCallError == 0;
}

bool shouldContinuePushMessageAfterCancelCallback(int cancelCallbackResultCode)
{
    (void)cancelCallbackResultCode;
    return true;
}

bool shouldApplyPushMessageButtonGeometryForSkinChange(bool initialBuild)
{
    return initialBuild;
}

PushMessageLoginResult makePushMessageLoginResultFromLegacyCallback(
    int resultCode,
    const char* failReason,
    const char* sndaId,
    const char* ticket,
    const char* tgt,
    const char* inputUserId)
{
    PushMessageLoginResult result;
    result.errorCode = mapLegacySdoBaseResultCode(resultCode);
    result.success = result.errorCode == 0;
    result.continuePolling = result.errorCode == kPushMessageContinuePollingErrorCode;
    result.userCanceledOnMobile = result.errorCode == kPushMessageMobileCanceledErrorCode;
    result.errorMessage = failReason && *failReason ? ansiToWideForLegacyAuth(failReason) : std::wstring();
    result.sndaId = sndaId ? sndaId : "";
    result.ticket = ticket ? ticket : "";
    result.sessionId = tgt ? tgt : "";
    result.inputUserId = inputUserId ? inputUserId : "";
    return result;
}

bool shouldContinuePushMessagePolling(const PushMessageLoginResult& result)
{
    return result.continuePolling && !result.success && !result.userCanceledOnMobile;
}

PushMessageLoginState pushMessageStateForLoginResult(const PushMessageLoginResult& result)
{
    if (result.success) {
        return PushMessageLoginState::Success;
    }
    if (shouldContinuePushMessagePolling(result)) {
        return PushMessageLoginState::WaitingForMobileConfirm;
    }
    return PushMessageLoginState::Failed;
}

}
