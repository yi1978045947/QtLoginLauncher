# Protocol Browser Resource And Link Handling

Date: 2026-05-17

## Problem

The protocol dialog previously used the stable Qt/WinHTTP fallback path instead of enabling CEF OSR for the user-facing agreement window. That avoided the earlier Debug CEF blank/crash issue, but `QTextBrowser::setHtml()` alone does not behave like a full browser:

- Remote images in the protocol page were not fetched into `QTextDocument`.
- CSS background images such as the privacy page footer logo were not visible.
- Links inside protocol content were not consistently delegated to the system browser.
- The first privacy page could crash while scrolling because remote image loading had been placed inside `QTextBrowser::loadResource()`, which Qt can call during layout and paint.

## Change

- Added URL resolution helpers in `protocol_browser_style`.
- Replaced plain `QTextBrowser` with an internal `ProtocolTextBrowser` subclass.
- `ProtocolTextBrowser` now only handles link clicks. It does not perform network I/O during `loadResource()`.
- Remote image URL collection remains covered by tests, but image injection is disabled in the Qt fallback after it caused privacy-page access violations.
- Full remote image rendering is now handled by the CEF/real-webview path in both Debug and Release `QtLoginBrowser` instead of `QTextDocument::ImageResource` injection.
- Debug `sdologin` launches the Debug `qtlogin_browser.exe` in the same runtime directory.
- Debug `QtLoginBrowser` now pre-initializes CEF before `QApplication`, so it no longer defaults to Qt text rendering.
- CEF link navigation is intercepted in `CefWidgetClient`: user-clicked HTTP/HTTPS links are opened with the system browser and canceled inside the agreement window.
- Qt fallback links are handled through `anchorClicked` and opened externally.
- The privacy page `.inscribe-img` CSS background image is promoted to a normal `<img>` tag for Qt rich text rendering.

## Files Changed

- `src/sdologin/protocol_browser_dialog.cpp`
- `src/sdologin/protocol_browser_style.h`
- `src/sdologin/protocol_browser_style.cpp`
- `tests/sdologin_tests/main.cpp`

## Verification

Commands run:

```powershell
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin_tests
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target QtLoginBrowser sdologin
Start-Process .\qtlogin_rewrite\build_qt5_32\bin\Debug\qtlogin_browser.exe ...
```

Result:

- `sdologin_tests passed`
- `qtlogin_browser.exe` built successfully
- `sdologin.exe` built successfully
- Debug and Release `QtLoginBrowser` CEF smoke tests logged `CefInitialize succeeded`, `OnLoadEnd status=200`, and `content paint detected`.
