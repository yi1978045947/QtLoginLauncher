#pragma once

#include <string>

namespace qtlogin::sdologin {

constexpr int kCodeKeyContinuePollingErrorCode = -10515805;
constexpr int kCodeKeyExpiredErrorCode = -10515801;

enum class CodeKeyLoginState {
    Idle,
    LoadingCode,
    CodeReady,
    Scanned,
    Expired,
    Failed,
    Success,
};

struct CodeKeyLoginResult {
    bool success = false;
    bool continuePolling = false;
    bool scanned = false;
    bool expired = false;
    bool needsSecondFactor = false;
    int errorCode = 0;
    std::wstring errorMessage;
    std::string sndaId;
    std::string ticket;
    std::string sessionId;
    std::string inputUserId;
};

CodeKeyLoginResult makeCodeKeyLoginResultFromLegacyCallback(
    int resultCode,
    const char* failReason,
    const char* sndaId,
    const char* ticket,
    const char* tgt,
    const char* inputUserId,
    const char* isScanned);

bool shouldContinueCodeKeyPolling(const CodeKeyLoginResult& result);
CodeKeyLoginState codeKeyStateForResult(const CodeKeyLoginResult& result);
int normalizedCodeKeyRefreshClickIntervalMs(int configuredMs);
bool shouldAllowCodeKeyRefreshClick(long long lastAcceptedMs, long long nowMs, int intervalMs);

}
