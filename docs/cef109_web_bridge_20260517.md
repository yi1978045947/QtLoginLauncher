# CEF 109 web bridge

## Purpose

`QtLoginBrowser` now contains the first CEF 109 JavaScript bridge so future web pages can exchange messages with the native Qt/C++ side.

## Project ownership

In the Visual Studio solution, Qt-to-web communication belongs to:

- VS folder: `03_qtlogin_internal`
- VS project: `QtLoginBrowser`
- CMake target: `QtLoginBrowser`
- Output process: `qtlogin_browser.exe`

The main source files are:

- `src/qtlogin_browser/main.cpp`: standalone browser process entry, command-line parsing, owner window attachment.
- `src/sdologin/cef_offscreen_browser.cpp`: CEF 109 off-screen browser widget, native-to-web and web-to-native dispatch.
- `src/sdologin/cef_web_bridge.cpp`: shared bridge object/function/message names.
- `src/sdologin/protocol_browser_dialog.cpp`: protocol dialog integration. Debug and Release `QtLoginBrowser` both use CEF 109 off-screen rendering by default for agreement pages.

`sdologin` launches this process through `src/sdologin/protocol_browser_launcher.cpp`.

The protocol agreement window uses CEF 109 in both Debug and Release browser processes. The browser process pre-initializes CEF before creating `QApplication`, and the agreement dialog no longer switches to the old Qt text fallback when CEF is slow to paint.

## JavaScript API

CEF injects this object into each page:

```javascript
window.qtlogin.postMessage(name, payload)
window.qtlogin.onMessage(function (name, payload) {
  // native to web message
})
```

Both `name` and `payload` are currently strings. Use JSON text for structured payloads.

Example:

```javascript
window.qtlogin.onMessage(function (name, payload) {
  console.log('native message', name, payload)
})

window.qtlogin.postMessage('pageReady', JSON.stringify({ path: location.href }))
```

## Native side

`CefOffscreenBrowserWidget` exposes:

```cpp
void setWebMessageHandler(std::function<void(const QString& name, const QString& payload)> handler);
void sendMessageToWeb(const QString& name, const QString& payload);
```

Current protocol dialog wiring logs web messages and sends a simple `nativeAck` response. Later business pages can replace that handler with login/payment-specific dispatch.

## Message names

Shared constants live in:

- `src/sdologin/cef_web_bridge.h`
- `src/sdologin/cef_web_bridge.cpp`

Browser process message names:

- `qtlogin.webToNative`
- `qtlogin.nativeToWeb`

## Notes

`sdologin` now launches the `qtlogin_browser.exe` from its own runtime directory. Debug `sdologin` uses Debug `QtLoginBrowser`, and Release `sdologin` uses Release `QtLoginBrowser`; both run CEF 109 off-screen rendering.
