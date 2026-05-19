#include "window_embed_style.h"

namespace qtlogin::sdologin {

LONG_PTR makeEmbeddedChildStyle(LONG_PTR style)
{
    style |= WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    return style;
}

LONG_PTR makeEmbeddedChildExStyle(LONG_PTR exStyle)
{
    exStyle &= ~WS_EX_APPWINDOW;
    exStyle &= ~WS_EX_LAYERED;
    exStyle &= ~WS_EX_TOOLWINDOW;
    return exStyle;
}

bool shouldMaskEmbeddedWindowBackground(bool embedWindow, bool hasAlphaChannel)
{
    return embedWindow && hasAlphaChannel;
}

bool shouldRemoveEmbeddedWindowAlpha(bool embedWindow, bool hasAlphaChannel)
{
    return embedWindow && hasAlphaChannel;
}

int embeddedWindowAlphaCutoff()
{
    return 250;
}

bool shouldCloseForMissingEmbeddedOwner(bool embedWindow, HWND ownerHwnd, bool ownerWindowExists)
{
    return embedWindow && ownerHwnd && !ownerWindowExists;
}

bool shouldAllowLoginWindowDrag(bool embedWindow)
{
    return !embedWindow;
}

}
