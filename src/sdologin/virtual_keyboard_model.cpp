#include "virtual_keyboard_model.h"

#include <algorithm>
#include <numeric>
#include <random>

namespace qtlogin::sdologin {
namespace {

int rowForIndex(size_t index)
{
    if (index < 14) {
        return 0;
    }
    if (index < 27) {
        return 1;
    }
    if (index < 39) {
        return 2;
    }
    return 3;
}

int columnForPosition(size_t position)
{
    if (position < 14) {
        return static_cast<int>(position);
    }
    if (position < 27) {
        return static_cast<int>(position - 14);
    }
    if (position < 39) {
        return static_cast<int>(position - 27);
    }
    return static_cast<int>(position - 39);
}

}

const std::vector<LegacyKeyboardKey>& legacyKeyboardKeys()
{
    static const std::vector<LegacyKeyboardKey> keys = [] {
        const char normal[] = "`1234567890-=\\qwertyuiop[]asdfghjkl;'zxcvbnm,./";
        const char shifted[] = "~!@#$%^&*()_+|QWERTYUIOP{}ASDFGHJKL:\"ZXCVBNM<>?";
        std::vector<LegacyKeyboardKey> result;
        result.reserve(47);
        for (size_t i = 0; i < 47; ++i) {
            result.push_back({normal[i], shifted[i], rowForIndex(i)});
        }
        return result;
    }();
    return keys;
}

std::vector<size_t> randomizedLegacyKeyboardOrder(uint32_t seed)
{
    std::vector<size_t> order(legacyKeyboardKeys().size());
    std::iota(order.begin(), order.end(), 0);
    std::mt19937 engine(seed);
    std::shuffle(order.begin(), order.begin() + 14, engine);
    std::shuffle(order.begin() + 14, order.begin() + 27, engine);
    std::shuffle(order.begin() + 27, order.begin() + 39, engine);
    std::shuffle(order.begin() + 39, order.end(), engine);
    return order;
}

bool keyboardOrderKeepsLegacyRows(const std::vector<size_t>& order)
{
    const auto& keys = legacyKeyboardKeys();
    if (order.size() != keys.size()) {
        return false;
    }
    std::vector<bool> seen(keys.size(), false);
    for (size_t i = 0; i < order.size(); ++i) {
        if (order[i] >= keys.size() || seen[order[i]]) {
            return false;
        }
        seen[order[i]] = true;
        if (keys[order[i]].row != rowForIndex(i)) {
            return false;
        }
    }
    return true;
}

LegacyRect legacyKeyboardWindowRect(double scaleFactor)
{
    return scaleLegacyRect({0, 0, 489, 208}, scaleFactor);
}

LegacyRect legacyKeyboardKeyRectForPosition(size_t position, double scaleFactor)
{
    if (position >= legacyKeyboardKeys().size()) {
        return {};
    }

    static const int rowY[] = {18, 50, 82, 114};
    static const int rowX[] = {18, 34, 50, 66};
    const int row = rowForIndex(position);
    const int column = columnForPosition(position);
    return scaleLegacyRect({rowX[row] + column * 28, rowY[row], 26, 22}, scaleFactor);
}

LegacyRect legacyKeyboardControlRect(LegacyKeyboardControl control, double scaleFactor)
{
    switch (control) {
    case LegacyKeyboardControl::Backspace:
        return scaleLegacyRect({402, 82, 66, 34}, scaleFactor);
    case LegacyKeyboardControl::Shift:
        return scaleLegacyRect({402, 114, 66, 34}, scaleFactor);
    case LegacyKeyboardControl::Enter:
        return scaleLegacyRect({310, 150, 92, 34}, scaleFactor);
    case LegacyKeyboardControl::Close:
        return scaleLegacyRect({441, 162, 25, 25}, scaleFactor);
    case LegacyKeyboardControl::Hint:
        return scaleLegacyRect({10, 164, 260, 22}, scaleFactor);
    }
    return {};
}

}
