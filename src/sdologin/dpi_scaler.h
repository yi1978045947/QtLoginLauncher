#pragma once

#include <string>

namespace qtlogin::sdologin {

struct LegacyRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

double legacyDpiScaleFactorFromDpi(int dpi);
double legacyDpiScaleFactorFromPercent(int percent);
double legacyDpiScaleFactorFromEnvironment();
int currentSystemDpiForLegacyScaling();
double chooseLegacyDpiScaleFactor(double requestedScale, double environmentScale, int systemDpi);
int scaleLegacyPixel(int value, double scaleFactor);
LegacyRect scaleLegacyRect(const LegacyRect& rect, double scaleFactor);
std::string scaleLegacyStyleSheetPixels(const std::string& styleSheet, double scaleFactor);

}
