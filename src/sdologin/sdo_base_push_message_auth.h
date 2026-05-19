#pragma once

#include <functional>
#include <memory>

#include "push_message_login_model.h"
#include "sdo_base_password_auth.h"

namespace qtlogin::sdologin {

using PushMessageSendCompletion = std::function<void(const PushMessageSendResult& result)>;
using PushMessageLoginCompletion = std::function<void(const PushMessageLoginResult& result)>;

class SdoBasePushMessageAuthenticator final {
public:
    explicit SdoBasePushMessageAuthenticator(SdoBaseAuthConfig config);
    ~SdoBasePushMessageAuthenticator();

    SdoBasePushMessageAuthenticator(const SdoBasePushMessageAuthenticator&) = delete;
    SdoBasePushMessageAuthenticator& operator=(const SdoBasePushMessageAuthenticator&) = delete;

    void startLogin(const PushMessageLoginRequest& request, PushMessageSendCompletion completion);
    void pollLogin(PushMessageLoginCompletion completion);
    void cancel();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
