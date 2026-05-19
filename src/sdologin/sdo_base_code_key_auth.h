#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "code_key_login_model.h"
#include "sdo_base_password_auth.h"

namespace qtlogin::sdologin {

struct CodeKeyImageResult {
    bool success = false;
    int errorCode = 0;
    std::wstring errorMessage;
    std::vector<unsigned char> imageBytes;
};

using CodeKeyImageCompletion = std::function<void(const CodeKeyImageResult& result)>;
using CodeKeyLoginCompletion = std::function<void(const CodeKeyLoginResult& result)>;

class SdoBaseCodeKeyAuthenticator final {
public:
    explicit SdoBaseCodeKeyAuthenticator(SdoBaseAuthConfig config);
    ~SdoBaseCodeKeyAuthenticator();

    SdoBaseCodeKeyAuthenticator(const SdoBaseCodeKeyAuthenticator&) = delete;
    SdoBaseCodeKeyAuthenticator& operator=(const SdoBaseCodeKeyAuthenticator&) = delete;

    void requestCodeImage(int maxSize, CodeKeyImageCompletion completion);
    void pollLogin(int maxSize, CodeKeyLoginCompletion completion);
    void clearCodeKey();
    void cancel();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
