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
9. On exit, allow the main window message stack to unwind, then call `ISDOADx9::Finalize`, release modules, call `igwTerminal`, and unload the SDK DLL.

`ShowLoginDialog` runs on one joinable worker thread. The demo no longer detaches login threads, because unloading
`sdologinsdk.dll` while a detached thread is still inside `ISDOAApp::ShowLoginDialog` can crash in
`sdologinsdk.dll_unloaded`. During shutdown the demo calls `igwTerminal`, waits for the login thread to return, and only
then releases module pointers and calls `FreeLibrary`.

2026-05-19 shutdown crash note:

`ISDOADx9::Initialize(..., hookGameWnd=true)` installs a compatibility window procedure on the DX owner window so mouse
input reaches the embedded Qt login child. If the demo calls `FreeLibrary(sdologinsdk.dll)` directly inside `WM_DESTROY`,
the call stack can still be executing that SDK window procedure. Returning from the already-unloaded DLL produces the
Windows event `dx_login_demo.exe / sdologinsdk.dll_unloaded / 0x000291f4`. The demo now only posts quit from
`WM_DESTROY`; SDK terminal and `FreeLibrary` run after `GetMessage` exits, when the SDK window procedure has already
returned and `WM_NCDESTROY` has restored the original owner window procedure.

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

Additional shutdown verification used for the crash fix:

```powershell
$start=Get-Date
$p=Start-Process .\qtlogin_rewrite\build_qt5_32\bin\Debug\dx_login_demo.exe -WorkingDirectory .\qtlogin_rewrite\build_qt5_32\bin\Debug -PassThru
Start-Sleep -Seconds 8
$p.CloseMainWindow()
Start-Sleep -Seconds 5
Get-WinEvent -FilterHashtable @{LogName='Application'; StartTime=$start} |
  Where-Object { $_.Message -match 'sdologin|dx_login_demo|sdologinsdk|qtlogin_browser' }
```

Expected result: the demo exits with code 0 and no matching Windows Error Reporting/Application Error events are created.
