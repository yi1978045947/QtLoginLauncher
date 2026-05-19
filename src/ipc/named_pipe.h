#pragma once

#include <string>

#include <windows.h>

#include "protocol_types.h"

namespace qtlogin::ipc {

class PipeConnection {
public:
    PipeConnection() = default;
    explicit PipeConnection(HANDLE handle);
    ~PipeConnection();

    PipeConnection(const PipeConnection&) = delete;
    PipeConnection& operator=(const PipeConnection&) = delete;

    PipeConnection(PipeConnection&& other) noexcept;
    PipeConnection& operator=(PipeConnection&& other) noexcept;

    bool isValid() const;
    void close();
    bool send(const protocol::Message& message);
    bool receive(protocol::Message* message);

private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
};

class PipeServer {
public:
    bool listen(const std::wstring& pipeName, DWORD timeoutMs, PipeConnection* connection);
};

class PipeClient {
public:
    bool connect(const std::wstring& pipeName, DWORD timeoutMs, PipeConnection* connection);
};

std::wstring fullPipeName(const std::wstring& pipeName);

}
