# Qt Demo SDK Call Mapping

This document records how `samples/qt_login_demo` maps the old DirectX demo launcher calls into the Qt rewrite demo.

## Legacy Files Read

- `E:\AllLoginDemo\DirectX3D\DirectX3D\Launcher.h`
- `E:\AllLoginDemo\DirectX3D\DirectX3D\Launcher.cpp`

The old demo keeps a launcher singleton, dynamically loads the login entry DLL, initializes the SDK, gets the login/render modules, sets the owner window, then calls login, ticket, browser, payment, face verification, collect-user-message, double-login callback, DPI, and logout related APIs.

## Qt Demo Entry

Current demo source:

- `samples/qt_login_demo/main.cpp`

The demo now keeps a persistent SDK session for the window lifetime instead of calling `SDOLTerminal` immediately after one login. This makes it possible to test login first, then call `GetTicket`, `GetTicketForAppid`, payment, browser, logout, and terminal manually.

## Button To SDK API Map

| Qt demo button | SDK calls | Old launcher equivalent |
| --- | --- | --- |
| `初始化 SDK` | `LoadLibraryW("sdologinsdk.dll")`, `SDOLInitialize`, `SDOLGetModule(__uuidof(ISDOLLogin7))`, `SetOwnerWindow`, `SetLoginMode`, `SetGameClientType`, `ModifyServerId`, `RegisterEvent`, `SetDoubleLoginCallBack`, `KeyBoardMessageNotify` | `Initial`, `SetHwnd`, `SetLoginMode`, `SetOwnerWindow`, `SetDoubleLogin` |
| `显示登录框` | `ShowLoginDialog(loginCallback, 2026, 0)` | `ShowLoginDlg` and `LoginCallback` |
| `关闭登录框` | `CloseLoginDialog` | `CloseLoginDlg` |
| `GetTicket` | `GetTicket` | `GetTicket()` |
| `GetTicketForAppid` | `GetTicketForAppid(..., 1001)` | `GetTicket(int appId)` |
| `Logout` | `Logout` | `Logout` |
| `OpenWindow 网页` | `OpenWindow("HTML", "扫码支付", url, 0, 0, 465, 500, "center")` | `OpenUrl` |
| `GhomePay 支付` | `GhomePay(jsonExtend, payCallback)` | `PaymentUrl` and `OnPaymentStatus` |
| `人脸核验回调` | `OpenFaceVertifyDlg(faceVerifyCallback, "demo")` | `OpenFaceVertify` and `FaceVertifyCallback` |
| `实名资料回调` | `OpenCollectUserMsgDlg(collectUserMsgCallback, "demo")` | `OpenFaceCollectMsg` and `FaceCollectMsgCallback` |
| `SDOLTerminal` | `Release`, `SDOLTerminal`, `FreeLibrary` | `Ternimal` |

## SDK Compatibility Added For Demo

The SDK adapter now supports these previously stubbed methods enough for demo verification:

- `ISDOLLogin4::OpenWindow`
- `ISDOLLogin7::OpenActivityWindow`
- `ISDOLLogin7::OpenXinIeWindow`
- `ISDOLLogin7::GhomePay`
- `ISDOLLogin7::OpenFaceVertifyDlg`
- `ISDOLLogin7::OpenCollectUserMsgDlg`

`OpenWindow` and payment launch `qtlogin_browser.exe` from the SDK DLL directory. `GhomePay` parses the old demo JSON fields `areaid`, `productid`, `gameorder`, `groupid`, and `extend`, builds a payment page URL, opens it, and invokes the pay callback with the window-open result. This is still a demo-level payment window open path; it is not yet the full old order creation/query payment-result service flow.

## Test Coverage

`tests/sdk_module_tests/main.cpp` now verifies:

- `ISDOLLogin7` and DirectX compatibility modules are queryable.
- `OpenWindow` returns `SDOL_ERRORCODE_OK`.
- `GhomePay` returns `SDOL_ERRORCODE_OK`.
- `GhomePay` invokes the callback with `SDOL_ERRORCODE_OK`.

The test sets `QTLOGIN_DISABLE_BROWSER_LAUNCH=1` so it exercises the ABI and command construction path without opening a visible browser process.
