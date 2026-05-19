#include <windows.h>

#include <cstring>

#include "libUtil/SafeStore_i.h"

namespace {

template <typename T>
void safeRelease(T*& object)
{
    if (object) {
        object->Release();
        object = nullptr;
    }
}

bool copyResult(const char* value, char* output, int outputSize)
{
    if (!value || !output || outputSize <= 0) {
        return false;
    }
    const size_t length = std::strlen(value);
    if (length + 1 > static_cast<size_t>(outputSize)) {
        return false;
    }
    std::memcpy(output, value, length + 1);
    return true;
}

}

extern "C" __declspec(dllexport) int __stdcall QtLogin_MakeLegacyPasswordTextBase64(
    const char* plainPasswordAnsi,
    const char* dynamicKey,
    char* output,
    int outputSize)
{
    if (!plainPasswordAnsi || !dynamicKey || !output || outputSize <= 0) {
        return -1;
    }
    output[0] = '\0';

    ISBYTE* byteObject = nullptr;
    ISafeStore* safeStore = nullptr;
    IAlgorithm* algorithm = nullptr;

    if (FAILED(::CreateObject(reinterpret_cast<void**>(&byteObject), SBYTE_ID)) || !byteObject ||
        FAILED(::CreateObject(reinterpret_cast<void**>(&safeStore), SAFESTORE_ID)) || !safeStore ||
        FAILED(safeStore->QueryInterface(reinterpret_cast<void**>(&algorithm), ALGORITHM_ID)) || !algorithm) {
        safeRelease(byteObject);
        safeRelease(safeStore);
        safeRelease(algorithm);
        return -2;
    }

    for (const unsigned char* cursor = reinterpret_cast<const unsigned char*>(plainPasswordAnsi); *cursor; ++cursor) {
        byteObject->PutValue(*cursor);
        safeStore->PushBack(byteObject);
    }

    unsigned char encrypted[2048] = {0};
    UINT encryptedLength = sizeof(encrypted);
    const HRESULT hr = algorithm->RSA(
        encrypted,
        encryptedLength,
        const_cast<char*>(dynamicKey),
        static_cast<UINT>(std::strlen(dynamicKey)));

    safeRelease(byteObject);
    safeRelease(safeStore);
    safeRelease(algorithm);

    if (FAILED(hr)) {
        return -3;
    }
    return copyResult(reinterpret_cast<const char*>(encrypted), output, outputSize) ? 0 : -4;
}

extern "C" __declspec(dllexport) int __stdcall QtLogin_SdoaEncryptWithDynamicKey(
    const char* plain,
    int plainSize,
    const char* dynamicKey,
    char* output,
    int outputSize)
{
    if (!plain || plainSize < 0 || !dynamicKey || !output || outputSize <= 0) {
        return -1;
    }
    output[0] = '\0';

    char encrypted[256] = {0};
    const int ret = sdoa_encrypt(plain, static_cast<size_t>(plainSize), dynamicKey, encrypted);
    if (ret != 0 && encrypted[0] == '\0') {
        return ret;
    }
    return copyResult(encrypted, output, outputSize) ? 0 : -4;
}
