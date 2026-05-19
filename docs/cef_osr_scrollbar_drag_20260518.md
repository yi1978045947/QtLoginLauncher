# CEF OSR Scrollbar Drag Handling

Date: 2026-05-18

## Problem

Protocol pages rendered through CEF 109 off-screen rendering could scroll with the mouse wheel, but dragging the web page scrollbar did not work reliably.

## Root Cause

`CefOffscreenBrowserWidget` forwarded mouse positions to CEF, but the generated `CefMouseEvent` did not include current mouse button flags. During a scrollbar drag CEF received mouse move events without `EVENTFLAG_LEFT_MOUSE_BUTTON`, so the renderer treated them as hover movement instead of left-button dragging.

## Change

- Map Qt mouse button state to CEF event flags:
  - `Qt::LeftButton` -> `EVENTFLAG_LEFT_MOUSE_BUTTON`
  - `Qt::MiddleButton` -> `EVENTFLAG_MIDDLE_MOUSE_BUTTON`
  - `Qt::RightButton` -> `EVENTFLAG_RIGHT_MOUSE_BUTTON`
- Map Shift/Ctrl/Alt keyboard modifiers as well.
- Use the same modifier mapping for mouse move, mouse click, and wheel events.
- Explicitly grab the mouse on press and release it after the final button release, so dragging remains stable even if the cursor moves outside the widget while the button is held.

## Verification

Commands run:

```powershell
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target QtLoginBrowser
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Release --target QtLoginBrowser
```

Debug CEF smoke log:

```text
qtlogin cef: CefInitialize succeeded
qtlogin cef: CreateBrowser posted
qtlogin cef: OnLoadEnd status=200 https://we.sdoprofile.com/common/static/register/public/privacy_game.html
qtlogin cef: content paint detected
```
