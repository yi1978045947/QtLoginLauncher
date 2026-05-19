# Qt Login Demo Run Notes

## Current Verified Build

The current working path is Qt 5.15.2 `msvc2019` Win32 with Visual Studio 2019. This is the preferred line for Win7 and old 32-bit SDK compatibility.

```powershell
cmake -S qtlogin_rewrite -B qtlogin_rewrite/build_qt5_32 -G "Visual Studio 16 2019" -A Win32 `
  -DQTLOGIN_ENABLE_QT_UI=ON `
  -DQTLOGIN_ENABLE_CEF_OSR=ON `
  -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\msvc2019

cmake --build qtlogin_rewrite/build_qt5_32 --config Debug --target DXLoginDemo -- /m
```

The build target pulls in the SDK, sdologin process, SdoBaseClient merge, QtLoginBrowser, and the DX demo dependencies.

## CEF 109 Dependency

CEF binaries are not committed to Git because the packages are several GB and exceed GitHub's normal file limit. Put the matching package under:

```text
qtlogin_rewrite\CefProject\
```

Expected Win32 package:

```text
cef_binary_109.1.2+g2f7620c+chromium-109.0.5414.61_windows32_beta
```

See `CefProject/README.md` and `docs/CEF_OSR_PROTOCOL.md`.

## Run The DirectX SDK Integration Demo

Open:

```powershell
qtlogin_rewrite\build_qt5_32\bin\Debug\dx_login_demo.exe
```

The demo is a plain Win32 C++/Direct3D9 caller:

- creates a DX9 device and render window;
- loads `sdologinsdk.dll`;
- uses the old `SDOA4Client`/`SDOLGetModule` style ABI;
- queries `ISDOAApp` and `ISDOADx9`;
- initializes the DX adapter with `D3DPRESENT_PARAMETERS::hDeviceWindow`;
- calls the login dialog flow;
- receives login success/cancel callbacks and shows the returned user/session fields.

In DirectX attach mode, `sdologin` receives the DX window handle over IPC and reparents the Qt login window with Win32 `SetParent` plus child-window style flags. It should appear inside the DirectX demo window, not as a separate taskbar login window.

The DX demo also exposes buttons for legacy API smoke calls such as:

- `ShowLoginDialog`
- `Logout`
- `GetLoginState`
- `ModifyAppInfo`
- `OpenWindow`
- `ShowPaymentDialog`
- `OpenWidget`
- `CloseWidget`
- `OpenFaceVertifyDlg`
- `GhomePay`

## Legacy Qt Demo

This is kept as a convenience preview, but the DirectX demo above is the closer replacement for a real game/client integration.

```powershell
qtlogin_rewrite\build_qt5_32\bin\Debug\qt_login_demo.exe
```

## Smoke Tests

```powershell
qtlogin_rewrite\build_qt5_32\bin\Debug\protocol_tests.exe
qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe
qtlogin_rewrite\build_qt5_32\bin\Debug\sdk_module_tests.exe
qtlogin_rewrite\build_qt5_32\bin\Debug\config_tests.exe
qtlogin_rewrite\build_qt5_32\bin\Debug\ui_model_tests.exe
```

Expected successful outputs include:

```text
sdologin_tests passed
config_tests passed
```

## Config Files

The new config module reads plain XML/CFG files from:

```text
<sdologin.exe directory>\config\
```

It currently supports `clientinfo.xml/cfg`, `config.xml/cfg`, `version.xml/cfg`, `userinfo.xml/cfg`, `browser.xml/cfg`, `widget.xml/cfg`, and root-level `skintype.xml/cfg`.

The default copied `config/config.xml` includes:

```xml
<PrivacyPolicyUrl value="https://we.sdoprofile.com/common/static/register/public/privacy_game.html" />
<ServicerAgreementUrl value="https://we.sdoprofile.com/common/static/register/public/user_agreements.html" />
<SupplementaryRulesUrl value="https://mxd.web.sdo.com/web7/news/violation.html" />
```

## Win7 Release Note

Qt 5.15.x is the practical target for the Win7-compatible line. Build Win32 when using the old SdoBaseClient/SafeStore/SdoA4Client integration and the CEF 109 Win32 package.
