#include "legacy_password_crypto.h"

#include <array>

#include <windows.h>

#if (defined(_M_IX86) || defined(__i386__)) && defined(QTLOGIN_ENABLE_LEGACY_AUTH_LIBS)
#define QTLOGIN_HAS_LEGACY_AUTH_LIBS 1
#else
#define QTLOGIN_HAS_LEGACY_AUTH_LIBS 0
#endif

namespace qtlogin::sdologin {
namespace {

void setUnsupportedError(std::wstring* errorMessage)
{
    if (errorMessage) {
        *errorMessage = L"Legacy SafeStore/SdoBase static auth libraries are not linked in this VS2019 Qt build.";
    }
}

using MakePasswordTextFn = int(__stdcall*)(
    const char* plainPasswordAnsi,
    const char* dynamicKey,
    char* output,
    int outputSize);

using SdoaEncryptFn = int(__stdcall*)(
    const char* plain,
    int plainSize,
    const char* dynamicKey,
    char* output,
    int outputSize);

#if QTLOGIN_HAS_LEGACY_AUTH_LIBS
HMODULE cryptoBridgeModule()
{
    static HMODULE module = ::LoadLibraryW(L"qtlogin_sdobase_crypto.dll");
    return module;
}

template <typename T>
T loadBridgeFunction(const char* name, const char* stdcallDecoratedName)
{
    HMODULE module = cryptoBridgeModule();
    if (!module) {
        return nullptr;
    }
    FARPROC proc = ::GetProcAddress(module, name);
    if (!proc && stdcallDecoratedName) {
        proc = ::GetProcAddress(module, stdcallDecoratedName);
    }
    return reinterpret_cast<T>(proc);
}

void setBridgeError(std::wstring* errorMessage, const wchar_t* action, int code)
{
    if (errorMessage) {
        *errorMessage = std::wstring(action) + L" failed in qtlogin_sdobase_crypto.dll, code=" + std::to_wstring(code);
    }
}
#endif

}

bool makeLegacyPasswordTextBase64(
    const std::string& plainPasswordAnsi,
    const std::string& dynamicKey,
    std::string* output,
    std::wstring* errorMessage)
{
    if (!output) {
        return false;
    }
    output->clear();
#if QTLOGIN_HAS_LEGACY_AUTH_LIBS
    MakePasswordTextFn makePasswordText = loadBridgeFunction<MakePasswordTextFn>(
        "QtLogin_MakeLegacyPasswordTextBase64",
        "_QtLogin_MakeLegacyPasswordTextBase64@16");
    if (!makePasswordText) {
        if (errorMessage) {
            *errorMessage = L"Failed to load QtLogin_MakeLegacyPasswordTextBase64 from qtlogin_sdobase_crypto.dll.";
        }
        return false;
    }

    std::array<char, 2048> encrypted{};
    const int ret = makePasswordText(plainPasswordAnsi.c_str(), dynamicKey.c_str(), encrypted.data(), static_cast<int>(encrypted.size()));
    if (ret != 0) {
        setBridgeError(errorMessage, L"Legacy password RSA encryption", ret);
        return false;
    }
    output->assign(encrypted.data());
    if (output->empty()) {
        if (errorMessage) {
            *errorMessage = L"Legacy password RSA encryption returned an empty result.";
        }
        return false;
    }
    return true;
#else
    (void)plainPasswordAnsi;
    (void)dynamicKey;
    setUnsupportedError(errorMessage);
    return false;
#endif
}

bool sdoaEncryptWithDynamicKey(
    const std::string& plain,
    const std::string& dynamicKey,
    std::string* output,
    std::wstring* errorMessage)
{
    if (!output) {
        return false;
    }
    output->clear();
#if QTLOGIN_HAS_LEGACY_AUTH_LIBS
    SdoaEncryptFn encrypt = loadBridgeFunction<SdoaEncryptFn>(
        "QtLogin_SdoaEncryptWithDynamicKey",
        "_QtLogin_SdoaEncryptWithDynamicKey@20");
    if (!encrypt) {
        if (errorMessage) {
            *errorMessage = L"Failed to load QtLogin_SdoaEncryptWithDynamicKey from qtlogin_sdobase_crypto.dll.";
        }
        return false;
    }

    std::array<char, 512> encrypted{};
    const int ret = encrypt(plain.data(), static_cast<int>(plain.size()), dynamicKey.c_str(), encrypted.data(), static_cast<int>(encrypted.size()));
    if (ret != 0) {
        setBridgeError(errorMessage, L"sdoa_encrypt", ret);
        return false;
    }
    output->assign(encrypted.data());
    if (output->empty()) {
        if (errorMessage) {
            *errorMessage = L"sdoa_encrypt returned an empty result.";
        }
        return false;
    }
    return true;
#else
    (void)plain;
    (void)dynamicKey;
    setUnsupportedError(errorMessage);
    return false;
#endif
}

}
