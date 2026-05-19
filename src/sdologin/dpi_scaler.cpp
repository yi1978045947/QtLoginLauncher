#include "dpi_scaler.h"

#include <cstdlib>
#include <cmath>
#include <cctype>
#include <string>

#include <windows.h>

namespace qtlogin::sdologin {
namespace {

using GetDpiForSystemFn = UINT(WINAPI*)();

}

double legacyDpiScaleFactorFromDpi(int dpi)
{
    if (dpi <= 0) {
        return 1.0;
    }
    return static_cast<double>(dpi) / 96.0;
}

double legacyDpiScaleFactorFromPercent(int percent)
{
    if (percent <= 0) {
        return 1.0;
    }
    return static_cast<double>(percent) / 100.0;
}

double legacyDpiScaleFactorFromEnvironment()
{
    if (const char* scale = std::getenv("QTLOGIN_LEGACY_DPI_SCALE")) {
        const double value = std::strtod(scale, nullptr);
        if (value > 0.0) {
            return value;
        }
    }
    if (const char* percent = std::getenv("QTLOGIN_LEGACY_DPI_PERCENT")) {
        const int value = std::atoi(percent);
        if (value > 0) {
            return legacyDpiScaleFactorFromPercent(value);
        }
    }
    return 0.0;
}

int currentSystemDpiForLegacyScaling()
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto* fn = reinterpret_cast<GetDpiForSystemFn>(GetProcAddress(user32, "GetDpiForSystem"));
        if (fn) {
            const UINT dpi = fn();
            if (dpi > 0) {
                return static_cast<int>(dpi);
            }
        }
    }

    HDC screen = GetDC(nullptr);
    if (!screen) {
        return 96;
    }
    const int dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(nullptr, screen);
    return dpi > 0 ? dpi : 96;
}

double chooseLegacyDpiScaleFactor(double requestedScale, double environmentScale, int systemDpi)
{
    if (requestedScale > 0.0) {
        return requestedScale;
    }
    if (environmentScale > 0.0) {
        return environmentScale;
    }
    return legacyDpiScaleFactorFromDpi(systemDpi);
}

int scaleLegacyPixel(int value, double scaleFactor)
{
    if (scaleFactor <= 0.0) {
        scaleFactor = 1.0;
    }
    return static_cast<int>(std::lround(static_cast<double>(value) * scaleFactor));
}

LegacyRect scaleLegacyRect(const LegacyRect& rect, double scaleFactor)
{
    LegacyRect scaled;
    scaled.x = scaleLegacyPixel(rect.x, scaleFactor);
    scaled.y = scaleLegacyPixel(rect.y, scaleFactor);
    scaled.width = scaleLegacyPixel(rect.width, scaleFactor);
    scaled.height = scaleLegacyPixel(rect.height, scaleFactor);
    return scaled;
}

std::string scaleLegacyStyleSheetPixels(const std::string& styleSheet, double scaleFactor)
{
    std::string output;
    output.reserve(styleSheet.size());

    for (size_t i = 0; i < styleSheet.size();) {
        if (!std::isdigit(static_cast<unsigned char>(styleSheet[i]))) {
            output.push_back(styleSheet[i++]);
            continue;
        }

        const size_t begin = i;
        while (i < styleSheet.size() && std::isdigit(static_cast<unsigned char>(styleSheet[i]))) {
            ++i;
        }

        if (i + 1 < styleSheet.size() && styleSheet[i] == 'p' && styleSheet[i + 1] == 'x') {
            const int value = std::atoi(styleSheet.substr(begin, i - begin).c_str());
            output += std::to_string(scaleLegacyPixel(value, scaleFactor));
            output += "px";
            i += 2;
            continue;
        }

        output.append(styleSheet, begin, i - begin);
    }

    return output;
}

}
