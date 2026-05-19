# QtLoginLauncher

QtLoginLauncher is a VS2019/CMake rewrite of the legacy DuiLib `sdologin` login SDK shell.

The rewrite keeps the old external SDK ABI direction while replacing the UI and internal process architecture with Qt-based projects:

- `sdk` / `sdologinsdk.dll`: legacy-facing SDK export and adapter layer.
- `sdologin` / `sdologin.exe`: Qt login process, old-skin-inspired layout, password login, one-click login, QR login, protocol acceptance, account history, and embedded window behavior.
- `SdoLoginClient` / `SdoBaseClient.dll`: in-tree VS2019 service communication layer merged from the old `sdologinclient` code.
- `QtLoginBrowser` / `qtlogin_browser.exe`: external CEF 109 protocol/browser process.
- `DXLoginDemo`: Direct3D9 demo using the old `SDOA4Client`/`ISDOAApp`/`ISDOADx9` style integration.
- `QtLoginDemo` and `SDKCallerDemo`: lighter SDK smoke/demo entry points.

## Repository Layout

```text
assets/          default config and DuiLib skin reference resources
cmake/           CMake helper scripts
docs/            migration notes, architecture notes, and feature-specific records
include/         public SDK-compatible headers
samples/         DX, Qt, and SDK caller demos
source/          original skin resource archive reference
src/             rewrite source code
tests/           protocol/config/sdk/ui-model smoke tests
third_party/     merged SdoBaseClient communication code
CefProject/      local-only CEF 109 package directory, see CefProject/README.md
```

Generated directories such as `build_qt5_32/`, `build_qt5_64/`, `build_qt64/`, and extracted CEF binaries are ignored by Git.

## Build: Win7-Compatible Win32 Qt 5.15

Install:

- Visual Studio 2019 with Win32 C++ toolset.
- Qt 5.15.2 `msvc2019` 32-bit kit.
- CMake 3.20 or newer.
- Optional but recommended: CEF 109 Win32 package under `CefProject/`.

Configure and build:

```powershell
cmake -S . -B build_qt5_32 -G "Visual Studio 16 2019" -A Win32 `
  -DQTLOGIN_ENABLE_QT_UI=ON `
  -DQTLOGIN_ENABLE_CEF_OSR=ON `
  -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\msvc2019

cmake --build build_qt5_32 --config Debug --target DXLoginDemo -- /m
```

Run:

```powershell
.\build_qt5_32\bin\Debug\dx_login_demo.exe
```

The DX demo creates the legacy-style SDK call chain, loads `sdologinsdk.dll`, launches `sdologin.exe`, and embeds the Qt login window into the DirectX host window.

## Smoke Tests

```powershell
cmake --build build_qt5_32 --config Debug --target protocol_tests sdologin_tests sdk_module_tests config_tests ui_model_tests -- /m
.\build_qt5_32\bin\Debug\protocol_tests.exe
.\build_qt5_32\bin\Debug\sdologin_tests.exe
.\build_qt5_32\bin\Debug\sdk_module_tests.exe
.\build_qt5_32\bin\Debug\config_tests.exe
.\build_qt5_32\bin\Debug\ui_model_tests.exe
```

## Key Documents

- `RUN_QT_DEMO.md`: local build/run notes.
- `docs/vs_solution_project_map.md`: Visual Studio project grouping and target mapping.
- `docs/legacy_login_call_flows.md`: old login flow reference.
- `docs/password_login_runtime_flow_20260516.md`: password login/service call flow.
- `docs/code_key_login_rewrite_20260518.md`: QR login rewrite.
- `docs/push_message_login_rewrite_20260518.md`: one-click login rewrite.
- `docs/account_history_combo_20260518.md`: account history dropdown and config persistence.
- `docs/CEF_OSR_PROTOCOL.md`: CEF 109 off-screen protocol window integration.
- `docs/sdoa4client_dx_demo_20260518.md`: DX SDK demo and legacy interface layering.
- `docs/sdobaseclient_integration.md`: merged SdoBaseClient integration.

## Notes

- CEF binaries are local dependencies and are not stored in this repository.
- Runtime-generated user account data is written to `userinfo.xml` next to the running `sdologin.exe` config directory and should not be committed.
- The current compatibility target is Windows 7 or newer, VS2019, Qt 5.15.x, Win32.
