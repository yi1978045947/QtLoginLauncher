# Protocol Initial Acceptance Migration

Date: 2026-05-17

## Legacy behavior checked

- Old source: `trunk/src/Login/ContainerDlg.cpp`
- When the protocol checkbox changes from unchecked to checked and `bShowProtocol` is enabled, the old login shell opens `CProtocolDlg` with the privacy policy URL.
- Old source: `trunk/src/Login/ProtocolDlg.cpp`
- The old protocol dialog can receive `SetShowTimer(true)`. In that mode the agree button is disabled first, displays a countdown, and is enabled after the configured protocol timer expires.
- Old source: `trunk/src/Login/SDOLApp.cpp`
- `ShowLoginDialog` calls `CAuthenManager::AsyncUserPrivacyConfig(localPrivacyVersion, localServiceAgreementVersion)` before creating the login window.
- Old source: `trunk/src/Login/Authen.cpp`
- `AsyncUserPrivacyConfig` calls `SdoBase_UserPrivacyConfig(handle, "optimisepc", ...)`; the callback returns `privacyPolicyVersion` and `serviceAgreementVersion`.
- Old source: `trunk/src/Login/ConfigManager.cpp`
- `ServicerAgreementVersion` is stored when the privacy-config callback succeeds. `PrivacyPolicyVersion` is stored after the user accepts the required privacy dialog.

## Qt rewrite behavior

- `sdologin` now reads `UserInfo/PrivacyPolicyVersion`.
- `sdologin` now calls `SdoBase_UserPrivacyConfig` through the merged `SdoBaseClient` wrapper before building the login UI.
- If the server privacy version differs from the saved privacy-policy version, or if the request fails and the saved version is missing, the protocol checkbox starts unchecked.
- On the first unchecked-to-checked action, `LoginWindow` opens the privacy policy dialog automatically.
- That first automatic privacy dialog passes `agreeDelaySeconds` to `QtLoginBrowser`.
- `QtLoginBrowser` disables the agree button and shows the countdown before enabling `同意并返回`.
- While the protocol dialog process is open, `LoginWindow` shows a transparent modal blocker so the login UI cannot be clicked.
- When `QtLoginBrowser` exits with Accepted/0, `sdologin` writes the returned server `PrivacyPolicyVersion` to `config/userinfo.xml`.
- If the browser is closed or crashes before acceptance, the checkbox is restored to unchecked and no privacy version is written.
- Normal protocol link clicks still open the protocol pages immediately with no forced countdown.

## Files changed

- `src/sdologin/protocol_acceptance_model.h`
- `src/sdologin/protocol_acceptance_model.cpp`
- `src/sdologin/login_window.h`
- `src/sdologin/login_window.cpp`
- `src/sdologin/main.cpp`
- `src/sdologin/sdo_base_password_auth.h`
- `src/sdologin/sdo_base_password_auth.cpp`
- `src/config/config_manager.h`
- `src/config/config_manager.cpp`
- `src/sdologin/protocol_browser_launcher.h`
- `src/sdologin/protocol_browser_launcher.cpp`
- `src/qtlogin_browser/main.cpp`
- `src/sdologin/protocol_browser_dialog.h`
- `src/sdologin/protocol_browser_dialog.cpp`
- `tests/sdologin_tests/main.cpp`

## Verification

Commands run:

```powershell
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin_tests
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target config_tests
.\qtlogin_rewrite\build_qt5_32\bin\Debug\config_tests.exe
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin QtLoginBrowser
```

Result:

- `sdologin_tests passed`
- `config_tests passed`
- `sdologin.exe` built successfully
- `qtlogin_browser.exe` built successfully
