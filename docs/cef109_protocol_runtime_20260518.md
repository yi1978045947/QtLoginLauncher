# CEF109 Protocol Runtime Notes

Date: 2026-05-18

## What Was Found

The agreement window was not using CEF in the user-facing path. It was forced to the Qt/WinHTTP `QTextBrowser` fallback, which explains:

- Remote images did not render like a real browser.
- Complex protocol page links could crash or behave inconsistently.
- CSS/JS behavior from the original pages was incomplete.

After re-enabling CEF for the protocol window, the local Debug `qtlogin_browser.exe` still could not be used as the default browser process. `CefInitialize()` either hung during lazy initialization or exited during pre-initialization with `0xE0000008`. The Release `qtlogin_browser.exe` works with the same CEF 109 package.

Follow-up debugging on 2026-05-18 narrowed this down:

- A minimal `CefDebugProbe` executable can initialize Debug CEF 109 successfully, including windowless rendering and multi-threaded message loop.
- `QtLoginBrowser` Debug hung after `qtlogin cef: loadUrl`, before producing `cef_debug.log`.
- Extra tracing showed the process reached `CefInitialize begin`.
- The Debug browser links CEF Debug wrapper with `_ITERATOR_DEBUG_LEVEL=0`; Qt Debug code using `QString::toStdWString()` / `QString::toStdString()` in the CEF path can block or become unstable across that STL debug ABI boundary.
- Replacing those conversions with explicit UTF-8 / UTF-16 conversion helpers allowed Debug forced CEF mode to initialize and paint content.
- `QtLoginBrowser` now also follows the CEF Win32 sample by calling `CefRunMainWithPreferredStackSize()` at executable entry and linking with the CEF-recommended stack size.

## Current Runtime Rule

- Release `QtLoginBrowser` uses CEF 109 off-screen rendering by default.
- Debug `QtLoginBrowser` also uses CEF 109 off-screen rendering by default.
- CEF is pre-initialized before `QApplication` in both Debug and Release to avoid slow lazy initialization after the dialog is already visible.
- The protocol dialog no longer auto-switches to the Qt/WinHTTP `QTextBrowser` fallback when CEF is slow to paint.

## Link And Image Handling

- CEF renders protocol page images/CSS/HTML.
- CEF user-clicked HTTP/HTTPS links are intercepted and opened with the system browser.
- The agreement window cancels that internal CEF navigation so it stays on the original protocol page.

## Verification

Commands run:

```powershell
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target QtLoginBrowser sdologin sdologin_tests
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Release --target QtLoginBrowser
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin.exe --standalone-url=https://we.sdoprofile.com/common/static/register/public/privacy_game.html
```

Observed Release browser log:

```text
qtlogin cef: CefInitialize succeeded
qtlogin cef: OnLoadEnd status=200 https://we.sdoprofile.com/common/static/register/public/privacy_game.html
qtlogin cef: content paint detected
```

Observed Debug CEF browser log after the fix:

```text
cef preinit begin
qtlogin cef: CefInitialize begin
qtlogin cef: CefInitialize succeeded
cef preinit succeeded
qtlogin cef: CreateBrowser posted
qtlogin cef: OnLoadEnd status=200 https://we.sdoprofile.com/common/static/register/public/privacy_game.html
qtlogin cef: content paint detected
```
