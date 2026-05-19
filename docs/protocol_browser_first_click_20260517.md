# Protocol browser first-click handling

## Problem

When `sdologin` had been idle for a long time, the first click on Privacy Policy, User Agreement, or Supplementary Rules could look like it did nothing.

The existing log showed the button click did reach the launcher and `qtlogin_browser.exe` was created. The weak point was the process boundary:

- `sdologin` started `qtlogin_browser.exe` and immediately returned.
- The CEF/Qt browser process can be slow on cold start.
- The browser window had no owner window and could appear behind the DX/demo window.
- There was no immediate UI feedback in the login window after a successful process launch.

## Change

- Added `ProtocolBrowserLaunchOptions`.
- `sdologin` now passes its login window HWND to `qtlogin_browser.exe` with `--owner-hwnd=...`.
- The parent process calls `AllowSetForegroundWindow` for the browser process.
- `qtlogin_browser.exe` attaches its dialog to the owner HWND when possible, centers over that owner, and calls foreground activation on startup.
- The login window immediately shows `正在打开协议页面...` after the click, so CEF cold start is visible to the user.
- `qtlogin_browser.exe` writes a lightweight startup/exit line to `qtlogin_rewrite.log` without linking `QtLoginCommon`, because the CEF Debug wrapper uses `_ITERATOR_DEBUG_LEVEL=0`.

## Follow-up latency change

The first-click path was still perceived as slow because the login UI did not always repaint before the parent process synchronously created `qtlogin_browser.exe`.

Additional changes:

- `showProtocolDialog` now logs `protocol click`.
- It forces one short Qt event pass after showing the hint.
- It starts `qtlogin_browser.exe` from an 80 ms timer, giving the hint a chance to paint first.
- `LoginWindow` schedules a one-time `--prewarm` browser process 300 ms after creation.
- `--prewarm` is loader-only: it starts `qtlogin_browser.exe`, loads dependent DLLs, logs, and exits. It intentionally does not call `CefInitialize/CefShutdown`, because prewarming CEF directly without a visible browser process was observed to exit abnormally in the Debug CEF 109 build.

## Debug CEF stability note

Further investigation found two Debug-browser issues:

- `QString::fromStdWString` and `QString::fromStdString` can hang in `QtLoginBrowser` because that target uses `_ITERATOR_DEBUG_LEVEL=0` to match the CEF Debug wrapper.
- The CEF OSR path currently triggers a Visual C++ Debug Runtime heap corruption dialog after creating the protocol window.

Follow-up work moved the protocol agreement windows back to CEF 109 off-screen rendering for both Debug and Release:

- CEF is pre-initialized before `QApplication`.
- CEF-path `QString::toStdString()` and `QString::toStdWString()` conversions were replaced with explicit UTF-8/UTF-16 helpers.
- The protocol dialog no longer auto-switches to the Qt `QTextBrowser` fallback when CEF is slow to paint.
- The old debug environment switches are no longer needed for normal protocol windows.

## Legacy Qt fallback content loading

The first fallback implementation only showed the configured URL and CEF status. That made the protocol window open, but it did not show the actual Privacy Policy, User Agreement, or Supplementary Rules content.

This fallback path is now legacy and is only retained for builds where CEF is unavailable. Normal Debug and Release protocol windows use CEF 109 OSR. The legacy path performs a real HTTP/HTTPS request from `qtlogin_browser.exe`:

- The dialog shows a loading page immediately.
- A background WinHTTP request loads the configured protocol URL.
- Successful responses are decoded and displayed in the existing Qt `QTextBrowser`.
- The text browser receives the protocol URL as its base URL so relative resources and links resolve from the original page.
- Failures show a clear error page and are logged to `qtlogin_rewrite.log`.

WinHTTP is used instead of QtNetwork because the local Qt 5.15 Win32 package does not include OpenSSL DLLs in `bin`.

When `sdologin` opens `qtlogin_browser.exe`, it now passes `--dpi-scale=<scale>` from the already scaled login window. This keeps the protocol process aligned with the parent login UI instead of asking a second process to rediscover DPI independently.

For DX embedded login, the SDK now also passes the owner window DPI scale to `sdologin` as `legacyDpiScale`. This prevents `sdologin.exe` from falling back to 96 DPI on machines where the child process cannot rediscover the parent window's effective DPI reliably.

## Dynamic protocol content fixes

Two configured protocol pages are not static HTML documents. They return a shell page and rely on JavaScript to fill the visible content. CEF executes that page logic directly. The legacy Qt fallback kept explicit parsers for builds where CEF is unavailable:

The fallback loader now handles these pages explicitly:

- User Agreement: `https://we.sdoprofile.com/common/static/register/public/user_agreements.html`
  - The page contains an empty `#yonghuContent` container and loads the real text through JSONP.
  - `qtlogin_browser.exe` now requests `https://utility.sdo.com/agreement/content?...&fn_callback=callback`, parses `data.servicerAgreement`, and builds static HTML for `QTextBrowser`.
- Supplementary Rules: `https://mxd.web.sdo.com/web7/news/violation.html`
  - The page contains empty `Title`, `ptime`, and `content` containers and loads the real news content through `../Handler/NewsContent.ashx?id=350555`.
  - `qtlogin_browser.exe` now requests `https://mxd.web.sdo.com/web7/Handler/NewsContent.ashx?id=350555`, parses `data.Title`, `data.PublishDate`, and `data.Content`, and builds static HTML for `QTextBrowser`.

Normal Privacy Policy, User Agreement, and Supplementary Rules windows now stay on the CEF path.

## Verification

Run:

```powershell
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target QtLoginBrowser sdologin
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe
.\qtlogin_rewrite\build_qt5_32\bin\Debug\config_tests.exe
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdk_module_tests.exe
```

Optional smoke check:

```powershell
$p = Start-Process .\qtlogin_rewrite\build_qt5_32\bin\Debug\qtlogin_browser.exe -ArgumentList @('--title=ProtocolLaunchSmoke','--url=about:blank','--owner-hwnd=0') -PassThru
Start-Sleep -Seconds 3
Stop-Process -Id $p.Id -Force
```

Content smoke check:

```powershell
$p = Start-Process .\qtlogin_rewrite\build_qt5_32\bin\Debug\qtlogin_browser.exe -ArgumentList @('--title=ProtocolContentProbe','--url=https://we.sdoprofile.com/common/static/register/public/user_agreements.html','--owner-hwnd=0') -PassThru
Start-Sleep -Seconds 10
Get-Content .\qtlogin_rewrite\build_qt5_32\bin\Debug\qtlogin_rewrite.log -Tail 40
Stop-Process -Id $p.Id -Force
```

Expected log line:

```text
[protocol_dialog] text fallback request done ok=1 status=200 bytes=...
```

For dynamic pages, also expect:

```text
[protocol_dialog] sdo agreement jsonp request done ok=1 status=200 bytes=...
[protocol_dialog] sdo violation news request done ok=1 status=200 bytes=...
```
