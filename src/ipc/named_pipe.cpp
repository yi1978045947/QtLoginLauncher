#include "named_pipe.h"

#include <cstring>
#include <vector>

#include "protocol_frame.h"

namespace qtlogin::ipc {
namespace {

bool writeAll(HANDLE handle, const void* data, DWORD size)
{
    const uint8_t* cursor = static_cast<const uint8_t*>(data);
    DWORD remaining = size;
    while (remaining > 0) {
        DWORD written = 0;
        if (!WriteFile(handle, cursor, remaining, &written, nullptr) || written == 0) {
            return false;
        }
        cursor += written;
        remaining -= written;
    }
    return true;
}

bool readAll(HANDLE handle, void* data, DWORD size)
{
    uint8_t* cursor = static_cast<uint8_t*>(data);
    DWORD remaining = size;
    while (remaining > 0) {
        DWORD read = 0;
        if (!ReadFile(handle, cursor, remaining, &read, nullptr) || read == 0) {
            return false;
        }
        cursor += read;
        remaining -= read;
    }
    return true;
}

}

std::wstring fullPipeName(const std::wstring& pipeName)
{
    if (pipeName.rfind(LR"(\\.\pipe\)", 0) == 0) {
        return pipeName;
    }
    return LR"(\\.\pipe\)" + pipeName;
}

PipeConnection::PipeConnection(HANDLE handle)
    : handle_(handle)
{
}

PipeConnection::~PipeConnection()
{
    close();
}

PipeConnection::PipeConnection(PipeConnection&& other) noexcept
    : handle_(other.handle_)
{
    other.handle_ = INVALID_HANDLE_VALUE;
}

PipeConnection& PipeConnection::operator=(PipeConnection&& other) noexcept
{
    if (this != &other) {
        close();
        handle_ = other.handle_;
        other.handle_ = INVALID_HANDLE_VALUE;
    }
    return *this;
}

bool PipeConnection::isValid() const
{
    return handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr;
}

void PipeConnection::close()
{
    if (isValid()) {
        FlushFileBuffers(handle_);
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
}

bool PipeConnection::send(const protocol::Message& message)
{
    const std::vector<uint8_t> bytes = protocol::serializeMessage(message);
    return writeAll(handle_, bytes.data(), static_cast<DWORD>(bytes.size()));
}

bool PipeConnection::receive(protocol::Message* message)
{
    protocol::FrameHeader header;
    if (!readAll(handle_, &header, sizeof(header))) {
        return false;
    }
    if (header.magic != protocol::kFrameMagic || header.version != protocol::kProtocolVersion) {
        return false;
    }

    std::vector<uint8_t> bytes(sizeof(protocol::FrameHeader) + header.payloadSize);
    std::memcpy(bytes.data(), &header, sizeof(header));
    if (header.payloadSize > 0) {
        if (!readAll(handle_, bytes.data() + sizeof(protocol::FrameHeader), header.payloadSize)) {
            return false;
        }
    }
    return protocol::parseMessage(bytes, message);
}

bool PipeServer::listen(const std::wstring& pipeName, DWORD timeoutMs, PipeConnection* connection)
{
    if (!connection) {
        return false;
    }

    const std::wstring name = fullPipeName(pipeName);
    HANDLE pipe = CreateNamedPipeW(
        name.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        64 * 1024,
        64 * 1024,
        0,
        nullptr);

    if (pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    OVERLAPPED overlapped{};
    overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!overlapped.hEvent) {
        CloseHandle(pipe);
        return false;
    }

    BOOL connected = ConnectNamedPipe(pipe, &overlapped);
    DWORD error = connected ? ERROR_SUCCESS : GetLastError();
    bool ok = false;
    if (connected || error == ERROR_PIPE_CONNECTED) {
        ok = true;
    } else if (error == ERROR_IO_PENDING) {
        const DWORD waitResult = WaitForSingleObject(overlapped.hEvent, timeoutMs);
        if (waitResult == WAIT_OBJECT_0) {
            DWORD bytes = 0;
            ok = GetOverlappedResult(pipe, &overlapped, &bytes, FALSE) != FALSE;
        } else {
            CancelIo(pipe);
        }
    }

    CloseHandle(overlapped.hEvent);
    if (!ok) {
        CloseHandle(pipe);
        return false;
    }

    *connection = PipeConnection(pipe);
    return true;
}

bool PipeClient::connect(const std::wstring& pipeName, DWORD timeoutMs, PipeConnection* connection)
{
    if (!connection) {
        return false;
    }

    const std::wstring name = fullPipeName(pipeName);
    const DWORD start = GetTickCount();
    while (GetTickCount() - start <= timeoutMs) {
        HANDLE pipe = CreateFileW(
            name.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);
        if (pipe != INVALID_HANDLE_VALUE) {
            *connection = PipeConnection(pipe);
            return true;
        }

        const DWORD error = GetLastError();
        if (error != ERROR_PIPE_BUSY && error != ERROR_FILE_NOT_FOUND) {
            return false;
        }
        WaitNamedPipeW(name.c_str(), 100);
        Sleep(50);
    }
    return false;
}

}
