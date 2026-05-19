# GetSystemConfig startup integration

## Old sdologin reference

Old `trunk/src/Login/Authen.cpp` calls `CAuthenManager::AsyncGetClientConfig()` during startup when `clientinfo.xml` does not set `InitSwitch=True`.

The old flow is:

1. `CAuthenManager::Start()` calls `Initialize()`.
2. `Initialize()` calls `SdoBase_Initialize3`.
3. If `InitSwitch` is not `True`, it calls `SdoBase_SetGetClientConfigCallback` and `SdoBase_GetClientConfig`.
4. `OnGetClientConfig` stores the returned JSON through `CConfigManager::SetClientConfig`.
5. Login windows read the parsed config to adjust placeholder text and available login methods.

`SdoBaseClient` sends this as `/authen/v2/getSystemConfig?logintype=godown` and returns the response `data` string through the `SdoBase_GetClientConfigCallback`.

## Qt rewrite implementation

The Qt rewrite now has the same startup hook:

- `src/sdologin/sdo_base_password_auth.h`
  - Adds `StartupClientConfigResult`.
  - Exposes `SdoBasePasswordAuthenticator::fetchStartupClientConfig(int timeoutMs)`.
- `src/sdologin/sdo_base_password_auth.cpp`
  - Reuses the same `SdoBase_Initialize3` handle as password login.
  - Registers `SdoBase_SetGetClientConfigCallback`.
  - Calls `SdoBase_GetClientConfig`.
  - Waits for the callback with a 10 second default timeout.
  - Parses the returned config JSON into `qtlogin::config::ClientConfigModel`.
- `src/config/client_config_model.*`
  - Parses the server JSON fields used by the old `ConfigManager::SetClientConfig`.
  - Supports `loginMethodList` values:
    - `pwdLogin`
    - `pushMessageLogin`
    - `codeKeyLogin`
    - `safePhoneSmsLogin`
  - Parses `activationCodeHint` nested JSON.
- `src/sdologin/main.cpp`
  - Reads `InitSwitch` from config.
  - Sets `LoginWindowContext::fetchClientConfig`.
- `src/sdologin/login_window.*`
  - Fetches startup config before building UI.
  - Applies `supportMobileAccountOnlyTip` to account placeholders.
  - Applies `forbiddenStaticPwdTip` to the password placeholder.
  - Uses `loginMethodList` to hide unavailable password, one-click, and QR tabs.

## Current scope

This step intentionally only wires the startup configuration into low-risk UI behavior. It does not yet implement the old secondary flows controlled by those config values, such as real-name second confirmation, activation code dialogs, voice tips, or SMS service calls.

Those follow-up flows should be added on top of `ClientConfigModel` instead of reparsing raw JSON in window code.

## Verification

Run:

```powershell
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target config_tests sdologin
.\qtlogin_rewrite\build_qt5_32\bin\Debug\config_tests.exe
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdk_module_tests.exe
```

