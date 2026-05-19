# VS Solution Project Map

This document records the Visual Studio project names used by the Qt rewrite and how they map to the old login SDK solution. The CMake target names are intentionally aligned with the Visual Studio display names so the solution is easy to read. ABI-sensitive DLL/EXE output names are kept compatible with the old SDK layout through `OUTPUT_NAME`.

## Runtime Projects

| VS Folder | VS Display Name | CMake Target | Output | Old Project / Purpose |
| --- | --- | --- | --- | --- |
| `01_runtime` | `sdk` | `sdk` | `sdologinsdk.dll` | Old `sdk`; external ABI-compatible SDK DLL. |
| `01_runtime` | `sdologin` | `sdologin` | `sdologin.exe` | Old `sdologin`; login UI process, now Qt-based. |
| `01_runtime` | `SdoLoginClient` | `SdoLoginClient` | `SdoBaseClient.dll` | Imported VS2019 server communication DLL from `sdologinclient`. |
| `01_runtime` | `GameUpdate` | `GameUpdate` | `GameUpdate.dll` | Old `GameUpdate`; update entry DLL. |
| `01_runtime` | `updatehelper` | `updatehelper` | `update.exe` | Old `updatehelper`; helper process. |
| `01_runtime` | `update_entry` | `update_entry` | `sdologinentry.dll` | Update entry bridge in the rewrite. |

## SdoBaseClient Projects

| VS Folder | VS Display Name | CMake Target | Output | Purpose |
| --- | --- | --- | --- | --- |
| `02_sdobaseclient` | `libLoginClient` | `libLoginClient` | `libLoginClient.lib` | Copied `sdologinclient\trunk\libLoginClient` static library. |
| `02_sdobaseclient` | `libUtil` | `libUtil` | `libUtil.lib` | Copied `sdologinclient\trunk\libUtil` utility library. |
| `02_sdobaseclient` | `SafeStoreCryptoHelper` | `SafeStoreCryptoHelper` | `qtlogin_sdobase_crypto.dll` | Isolates `SafeStore.lib` password encryption from the Qt `/MD` process. |

## Qt Rewrite Internal Projects

| VS Folder | VS Display Name | CMake Target | Output | Purpose |
| --- | --- | --- | --- | --- |
| `03_qtlogin_internal` | `QtLoginCommon` | `QtLoginCommon` | `QtLoginCommon.lib` | Common Win32/string/log helpers. |
| `03_qtlogin_internal` | `QtLoginConfig` | `QtLoginConfig` | `QtLoginConfig.lib` | XML/default configuration loading. |
| `03_qtlogin_internal` | `QtLoginIPC` | `QtLoginIPC` | `QtLoginIPC.lib` | SDK and `sdologin` process IPC. |
| `03_qtlogin_internal` | `QtLoginProtocol` | `QtLoginProtocol` | `QtLoginProtocol.lib` | SDK/sdologin message protocol structures. |
| `03_qtlogin_internal` | `QtLoginUIModel` | `QtLoginUIModel` | `QtLoginUIModel.lib` | Login UI state models and validation. |
| `03_qtlogin_internal` | `QtLoginBrowser` | `QtLoginBrowser` | `qtlogin_browser.exe` | External CEF protocol/browser process. |

## Demo Projects

| VS Folder | VS Display Name | CMake Target | Output | Purpose |
| --- | --- | --- | --- | --- |
| `04_demos` | `DXLoginDemo` | `DXLoginDemo` | `dx_login_demo.exe` | DirectX demo integrating the SDK/login process. |
| `04_demos` | `QtLoginDemo` | `QtLoginDemo` | `qt_login_demo.exe` | Qt-only demo. |
| `04_demos` | `SDKCallerDemo` | `SDKCallerDemo` | `sdk_caller.exe` | Console SDK caller demo. |

## Tests

| VS Folder | VS Display Name | CMake Target | Purpose |
| --- | --- | --- | --- |
| `05_tests` | `config_tests` | `config_tests` | Configuration parser tests. |
| `05_tests` | `protocol_tests` | `protocol_tests` | IPC protocol tests. |
| `05_tests` | `sdologin_tests` | `sdologin_tests` | sdologin command/runtime tests. |
| `05_tests` | `sdk_module_tests` | `sdk_module_tests` | SDK export/runtime tests. |
| `05_tests` | `ui_model_tests` | `ui_model_tests` | UI model validation tests. |

## Notes

- Most runtime and internal targets now use readable target names directly, so Visual Studio project names match the CMake target names.
- `OUTPUT_NAME` controls the generated DLL/EXE file name and is intentionally kept compatible with the old SDK layout.
- `sdk` still outputs `sdologinsdk.dll`, and `QtLoginBrowser` still outputs `qtlogin_browser.exe`, so existing runtime launch/copy logic does not need to change.
- `SdoLoginClient` still outputs `SdoBaseClient.dll` and uses the copied original `SdoLoginClient.def`, so exported communication functions and ordinals remain unchanged.
