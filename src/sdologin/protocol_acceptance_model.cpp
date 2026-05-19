#include "protocol_acceptance_model.h"

namespace qtlogin::sdologin {

bool shouldRequireInitialProtocolReview(int savedPrivacyPolicyVersion)
{
    return savedPrivacyPolicyVersion < 0;
}

bool initialProtocolCheckboxChecked(bool protocolReviewRequired)
{
    return !protocolReviewRequired;
}

bool shouldOpenInitialPrivacyProtocol(
    bool protocolReviewRequired,
    bool wasChecked,
    bool isChecked,
    bool alreadyOpened)
{
    return protocolReviewRequired && !alreadyOpened && !wasChecked && isChecked;
}

int normalizedProtocolAgreeDelaySeconds(int configuredSeconds)
{
    return configuredSeconds > 0 ? configuredSeconds : 5;
}

bool shouldRequireProtocolReviewAfterServerConfig(
    int savedPrivacyPolicyVersion,
    bool serverConfigSucceeded,
    int serverPrivacyPolicyVersion)
{
    if (!serverConfigSucceeded) {
        return shouldRequireInitialProtocolReview(savedPrivacyPolicyVersion);
    }
    return serverPrivacyPolicyVersion != savedPrivacyPolicyVersion;
}

int acceptedPrivacyPolicyVersionForStorage(bool serverConfigSucceeded, int serverPrivacyPolicyVersion)
{
    return serverConfigSucceeded && serverPrivacyPolicyVersion >= 0 ? serverPrivacyPolicyVersion : -1;
}

bool shouldPersistInitialProtocolAcceptance(
    bool recordInitialAcceptance,
    int browserExitCode,
    int privacyPolicyVersionToStore)
{
    return recordInitialAcceptance && browserExitCode == 0 && privacyPolicyVersionToStore >= 0;
}

}
