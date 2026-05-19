#include <windows.h>

#include <cstdio>
#include <string>
#include <vector>

#include "include/cef_app.h"

namespace {

bool hasArg(int argc, char* argv[], const char* expected)
{
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == expected) {
            return true;
        }
    }
    return false;
}

std::wstring executableDirectory()
{
    std::vector<wchar_t> buffer(MAX_PATH);
    DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (size == buffer.size()) {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    if (size == 0) {
        return L".";
    }

    std::wstring path(buffer.data(), size);
    const size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L".";
    }
    return path.substr(0, pos);
}

void printWide(const wchar_t* label, const std::wstring& value)
{
    std::printf("%ls%ls\n", label, value.c_str());
    std::fflush(stdout);
}

int runProbe(int argc, char* argv[])
{
    std::printf("probe: begin\n");
    std::fflush(stdout);

    CefMainArgs mainArgs(GetModuleHandleW(nullptr));
    std::printf("probe: before CefExecuteProcess\n");
    std::fflush(stdout);
    const int processExit = CefExecuteProcess(mainArgs, nullptr, nullptr);
    std::printf("probe: after CefExecuteProcess exit=%d\n", processExit);
    std::fflush(stdout);
    if (processExit >= 0) {
        return processExit;
    }

    const std::wstring dir = executableDirectory();
    printWide(L"probe: exe dir=", dir);

    CefSettings settings;
    settings.no_sandbox = true;
    settings.windowless_rendering_enabled = hasArg(argc, argv, "--windowless");
    settings.multi_threaded_message_loop = hasArg(argc, argv, "--mt");
    settings.log_severity = LOGSEVERITY_VERBOSE;

    const std::wstring logPath = dir + L"\\cef_debug_probe.log";
    const std::wstring cachePath = dir + L"\\cef_debug_probe_cache";
    CefString(&settings.log_file).FromWString(logPath);
    CefString(&settings.cache_path).FromWString(cachePath);

    std::printf(
        "probe: before CefInitialize windowless=%d mt=%d\n",
        settings.windowless_rendering_enabled,
        settings.multi_threaded_message_loop);
    std::fflush(stdout);

    const bool initialized = CefInitialize(mainArgs, settings, nullptr, nullptr);
    std::printf("probe: after CefInitialize initialized=%d\n", initialized ? 1 : 0);
    std::fflush(stdout);
    if (initialized) {
        CefShutdown();
        std::printf("probe: after CefShutdown\n");
        std::fflush(stdout);
    }
    return initialized ? 0 : 2;
}

}

int main(int argc, char* argv[])
{
#if defined(ARCH_CPU_32_BITS)
    if (!hasArg(argc, argv, "--no-fiber")) {
        std::printf("probe: before CefRunMainWithPreferredStackSize\n");
        std::fflush(stdout);
        const int fiberExit = CefRunMainWithPreferredStackSize(main, argc, argv);
        std::printf("probe: after CefRunMainWithPreferredStackSize exit=%d\n", fiberExit);
        std::fflush(stdout);
        if (fiberExit >= 0) {
            return fiberExit;
        }
    }
#endif
    return runProbe(argc, argv);
}
