# CEF 109 Protocol Page Integration

Protocol dialogs are now routed through `ProtocolBrowserDialog`.

- If `QTLOGIN_ENABLE_CEF_OSR` is enabled and the build is Win32, the dialog uses `CefOffscreenBrowserWidget`.
- `CefOffscreenBrowserWidget` creates a CEF 109 windowless browser with `SetAsWindowless`, paints CEF BGRA buffers into a Qt widget, and forwards mouse/wheel input to CEF.
- If the local CEF package cannot be linked, the same dialog opens with a clear fallback message and still shows the configured URL.

## Local Dependency Constraint

The local CEF package path is:

`CefProject/cef_binary_109.1.2+g2f7620c+chromium-109.0.5414.61_windows32_beta`

Its `libcef.dll` PE machine type is `0x014C`, which is x86/Win32. The current local Qt installation is only:

`C:/Qt/6.5.3/msvc2019_64`

That combination cannot link CEF into `sdologin.exe` in-process because the Qt application is x64 and CEF is x86.

## How To Enable Real CEF OSR

Use either one of these dependency sets:

1. Build the whole Qt login rewrite as Win32 with a Win32 Qt build and a local CEF 109 windows32 package.
2. Keep the current x64 Qt build, but add a matching CEF 109 windows64 package and point `QTLOGIN_CEF109_ROOT` to it.

Then configure with:

```powershell
cmake -S qtlogin_rewrite -B qtlogin_rewrite/build_qt32 -G "Visual Studio 16 2019" -A Win32 -DQTLOGIN_ENABLE_QT_UI=ON -DQTLOGIN_ENABLE_CEF_OSR=ON -DCMAKE_PREFIX_PATH=C:/Qt/<win32-qt>/lib/cmake
cmake --build qtlogin_rewrite/build_qt32 --config Release --target sdologin dx_login_demo
```

For final Win7 support, prefer Qt 5.15.x Win32 plus a matching local CEF 109 Win32 package.
