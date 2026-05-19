#include "code_key_login_model.h"

#include "legacy_password_auth.h"

namespace qtlogin::sdologin {

CodeKeyLoginResult makeCodeKeyLoginResultFromLegacyCallback(
    int resultCode,
    const char* failReason,
    const char* sndaId,
    const char* ticket,
    const char* tgt,
    const char* inputUserId,
    const char* isScanned)
{
    CodeKeyLoginResult result;
    result.errorCode = mapLegacySdoBaseResultCode(resultCode);
    result.success = result.errorCode == 0;
    result.continuePolling = result.errorCode == kCodeKeyContinuePollingErrorCode;
    result.scanned = result.continuePolling && isScanned && std::string(isScanned) == "1";
    result.expired = result.errorCode == kCodeKeyExpiredErrorCode;
    result.errorMessage = failReason && *failReason ? ansiToWideForLegacyAuth(failReason) : std::wstring();
    result.sndaId = sndaId ? sndaId : "";
    result.ticket = ticket ? ticket : "";
    result.sessionId = tgt ? tgt : "";
    result.inputUserId = inputUserId ? inputUserId : "";
    return result;
}

bool shouldContinueCodeKeyPolling(const CodeKeyLoginResult& result)
{
    return result.continuePolling && !result.success && !result.expired;
}

CodeKeyLoginState codeKeyStateForResult(const CodeKeyLoginResult& result)
{
    if (result.success) {
        return CodeKeyLoginState::Success;
    }
    if (result.expired) {
        return CodeKeyLoginState::Expired;
    }
    if (result.scanned) {
        return CodeKeyLoginState::Scanned;
    }
    if (shouldContinueCodeKeyPolling(result)) {
        return CodeKeyLoginState::CodeReady;
    }
    return CodeKeyLoginState::Failed;
}

int normalizedCodeKeyRefreshClickIntervalMs(int configuredMs)
{
    return configuredMs > 0 ? configuredMs : 1500;
}

bool shouldAllowCodeKeyRefreshClick(long long lastAcceptedMs, long long nowMs, int intervalMs)
{
    if (lastAcceptedMs < 0) {
        return true;
    }
    return nowMs - lastAcceptedMs >= normalizedCodeKeyRefreshClickIntervalMs(intervalMs);
}

}
