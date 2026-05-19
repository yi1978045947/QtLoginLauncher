#include "qt_dpi_config.h"

#include <QCoreApplication>
#include <QtGlobal>

#include <windows.h>

namespace qtlogin::sdologin {
namespace {

using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(HANDLE);

#ifndef DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE reinterpret_cast<HANDLE>(-2)
#endif

bool setDpiAwarenessContext(HANDLE context)
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32) {
        return false;
    }

    auto* fn = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
        GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    return fn && fn(context);
}

bool setSystemDpiAwarenessContext()
{
    return setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
}

}

void configureQtLoginDpi()
{
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", qtLoginAutoScreenScaleFactorEnvValue());
    qputenv("QT_ENABLE_HIGHDPI_SCALING", qtLoginHighDpiScalingEnvValue());
    qputenv("QT_SCALE_FACTOR", "1");
    qputenv("QT_FONT_DPI", "96");

    QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling, true);
#endif

    if (!setSystemDpiAwarenessContext()) {
        SetProcessDPIAware();
    }
}

}
