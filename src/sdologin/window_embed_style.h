#pragma once

#include <windows.h>

namespace qtlogin::sdologin {

LONG_PTR makeEmbeddedChildStyle(LONG_PTR style);
LONG_PTR makeEmbeddedChildExStyle(LONG_PTR exStyle);
bool shouldMaskEmbeddedWindowBackground(bool embedWindow, bool hasAlphaChannel);
bool shouldRemoveEmbeddedWindowAlpha(bool embedWindow, bool hasAlphaChannel);
int embeddedWindowAlphaCutoff();
bool shouldCloseForMissingEmbeddedOwner(bool embedWindow, HWND ownerHwnd, bool ownerWindowExists);
bool shouldAllowLoginWindowDrag(bool embedWindow);

}
