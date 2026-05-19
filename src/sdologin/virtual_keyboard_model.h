#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "dpi_scaler.h"

namespace qtlogin::sdologin {

struct LegacyKeyboardKey {
    char normal = '\0';
    char shifted = '\0';
    int row = 0;
};

enum class LegacyKeyboardControl {
    Backspace,
    Shift,
    Enter,
    Close,
    Hint,
};

const std::vector<LegacyKeyboardKey>& legacyKeyboardKeys();
std::vector<size_t> randomizedLegacyKeyboardOrder(uint32_t seed);
bool keyboardOrderKeepsLegacyRows(const std::vector<size_t>& order);
LegacyRect legacyKeyboardWindowRect(double scaleFactor);
LegacyRect legacyKeyboardKeyRectForPosition(size_t position, double scaleFactor);
LegacyRect legacyKeyboardControlRect(LegacyKeyboardControl control, double scaleFactor);

}
