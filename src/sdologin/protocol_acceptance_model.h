#pragma once

namespace qtlogin::sdologin {

bool shouldRequireInitialProtocolReview(int savedPrivacyPolicyVersion);
bool initialProtocolCheckboxChecked(bool protocolReviewRequired);
bool shouldOpenInitialPrivacyProtocol(
    bool protocolReviewRequired,
    bool wasChecked,
    bool isChecked,
    bool alreadyOpened);
int normalizedProtocolAgreeDelaySeconds(int configuredSeconds);
bool shouldRequireProtocolReviewAfterServerConfig(
    int savedPrivacyPolicyVersion,
    bool serverConfigSucceeded,
    int serverPrivacyPolicyVersion);
int acceptedPrivacyPolicyVersionForStorage(bool serverConfigSucceeded, int serverPrivacyPolicyVersion);
bool shouldPersistInitialProtocolAcceptance(
    bool recordInitialAcceptance,
    int browserExitCode,
    int privacyPolicyVersionToStore);

}
