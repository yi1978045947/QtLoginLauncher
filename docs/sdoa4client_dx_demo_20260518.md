# SDOA4Client DX demo integration notes

## Goal

This change restores the old DX-facing SDK shape while keeping the newer Qt login process implementation:

- DX/game callers use `SDOA4Client.h` and `igwInitialize / igwGetModule / igwTerminal`.
- The login window is still implemented by the Qt `sdologin.exe` process.
- `ISDOLLogin7` remains available, but it is treated as a login/business extension interface rather than the primary DX integration path.

## SDK ABI additions

New compatibility headers were added under `include/`:

- `SDOA4Client.h`
- `SDOADx8.h`
- `SDOADx9.h`
- `SDOADx11.h`

`sdologinsdk.dll` now exposes the old SDOA4 signatures:

```cpp
int  WINAPI igwInitialize(DWORD dwSdkVersion, const AppInfo* pAppInfo);
bool WINAPI igwGetModule(REFIID riid, void** intf);
int  WINAPI igwTerminal();
```

The previous `SDOLInitialize / SDOLGetModule / SDOLTerminal` exports are still present for the newer login-window interface.

## Module mapping

`igwGetModule` now supports:

- `ISDOADx8`
- `ISDOADx9`
- `ISDOADx11`
- `ISDOAApp`
- `ISDOAApp2`
- `ISDOAAppUtils`
- `ISDOAClientService`

`SDOLGetModule` still supports:

- `ISDOLLogin` through `ISDOLLogin7`
- DX compatibility modules for existing internal tests and transitional callers

## ISDOAApp behavior

`SdoaAppAdapter` bridges old `ISDOAApp/ISDOAApp2` calls onto the current Qt runtime.

Implemented bridge behavior:

- `ShowLoginDialog` launches the Qt `sdologin.exe` flow and converts `SDOLLoginResult` to old narrow-string `LoginResult`.
- `ModifyAppInfo` maps old `AppInfo` to current `SDOLAppInfo`.
- `Logout` clears current runtime login state.
- `GetLoginState` returns old SDOA login-state constants.
- `ShowPaymentDialog` opens a browser window through the existing SDK browser launcher.
- `OpenWindow` opens a browser window through the existing SDK browser launcher.
- `OpenWidget / CloseWidget / WidgetExists` maintain simple compatibility state for manual/demo callers.
- `LoginDirect` records a direct session in the runtime and marks the old login state as OK.
- `GetClientSignature` returns a deterministic placeholder signature for ABI compatibility.

Minimal no-op compatibility was also added for `ISDOAAppUtils` and key/value state for `ISDOAClientService`.

## DXLoginDemo flow

`DXLoginDemo` now follows the old DX integration chain:

1. Load `sdologinsdk.dll`.
2. Resolve `igwInitialize / igwGetModule / igwTerminal`.
3. Call `igwInitialize(SDOA_SDK_VERSION, &AppInfo)`.
4. Query `ISDOADx9` with `igwGetModule`.
5. Call `ISDOADx9::Initialize(device, presentParams, true)`.
6. Query `ISDOAApp2` with `igwGetModule`.
7. Use `ISDOAApp2::ShowLoginDialog` for the main login button and auto-login path.
8. Query `ISDOLLogin7` separately through `SDOLGetModule` only for extension buttons such as face verify, GhomePay, GetTicket, and the legacy login-window OpenWindow.
9. On exit, call `ISDOADx9::Finalize`, release modules, then call `igwTerminal`.

## Demo buttons

The DX demo now creates native Win32 buttons for manual API checks:

- `ShowLoginDialog`
- `Logout`
- `GetLoginState`
- `ModifyAppInfo`
- `OpenWindow`
- `Payment`
- `OpenWidget`
- `CloseWidget`
- `FaceVerify`
- `GhomePay`
- `GetTicket`
- `ISDOL OpenWindow`

The first eight use old `ISDOAApp/ISDOAApp2` where applicable. The last extension operations use `ISDOLLogin7` intentionally, so the demo clearly separates old DX/game integration from login-extension APIs.

## Embedded login position

The demo keeps the API buttons on the left side and moves the embedded Qt login panel to the right side of the DX client area before calling `ISDOAApp::ShowLoginDialog`.

The position is calculated by `dxDemoLoginDialogPosition(clientWidth, clientHeight, legacyDpiScale)` in `samples/dx_login_demo/dx_demo_behavior.cpp`.

`DXLoginDemo` applies that position with `ISDOLLogin7::MoveLoginDialog` before the SDOA login call. This keeps the public DX path as `igw* + ISDOAApp`, while using the existing login-extension method only to pass the embedded-window position to the shared runtime.

## Verification

Run from `F:\vs2019_code\Launcher\Release\测试\loginCode\QTLogin`:

```powershell
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target DXLoginDemo sdologin_tests sdk_module_tests
```

Then run from `qtlogin_rewrite\build_qt5_32\bin\Debug`:

```powershell
.\sdologin_tests.exe
.\sdk_module_tests.exe
```
