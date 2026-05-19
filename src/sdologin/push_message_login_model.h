#pragma once

#include <string>

namespace qtlogin::sdologin {

constexpr int kPushMessageContinuePollingErrorCode = -10516808;
constexpr int kPushMessageMobileCanceledErrorCode = -10516809;

enum class PushMessageLoginValidationCode {
    Ok,
    ProtocolRequired,
    EmptyAccount,
    InvalidMobileAccount,
};

struct PushMessageLoginRequest {
    std::wstring account;
    bool protocolAccepted = false;
    bool supportMobileAccountOnly = false;
};

struct PushMessageLoginValidationResult {
    PushMessageLoginValidationCode code = PushMessageLoginValidationCode::Ok;
    std::wstring message;
    std::wstring trimmedAccount;
};

enum class PushMessageLoginState {
    Idle,
    Sending,
    WaitingForMobileConfirm,
    Failed,
    Success,
};

struct PushMessageSendResult {
    bool success = false;
    int errorCode = 0;
    std::wstring errorMessage;
    std::string serialNumber;
};

struct PushMessageLoginResult {
    bool success = false;
    bool continuePolling = false;
    bool userCanceledOnMobile = false;
    bool needsSecondFactor = false;
    int errorCode = 0;
    std::wstring errorMessage;
    std::string sndaId;
    std::string ticket;
    std::string sessionId;
    std::string inputUserId;
};

PushMessageLoginValidationResult validatePushMessageLoginRequest(const PushMessageLoginRequest& request);
std::string pushMessageLoginScene(bool xpOrOlder);
int normalizedPushMessageRetryCountdownSeconds(int configuredSeconds);
bool shouldIssuePushMessageSendAfterCancel(bool sendAlreadyIssued);
bool shouldWaitForPushMessageCancelCallback(int cancelCallError);
bool shouldContinuePushMessageAfterCancelCallback(int cancelCallbackResultCode);
bool shouldApplyPushMessageButtonGeometryForSkinChange(bool initialBuild);
PushMessageLoginResult makePushMessageLoginResultFromLegacyCallback(
    int resultCode,
    const char* failReason,
    const char* sndaId,
    const char* ticket,
    const char* tgt,
    const char* inputUserId);
bool shouldContinuePushMessagePolling(const PushMessageLoginResult& result);
PushMessageLoginState pushMessageStateForLoginResult(const PushMessageLoginResult& result);

}
