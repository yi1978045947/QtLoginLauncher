# Legacy Login Call Flows

> Scope: old `trunk/src/Login` process flows that the Qt rewrite must preserve.  
> Focus: password login, one-click push login, QR login, SMS/captcha, secondary auth, SDK IPC callback.

## Shared Runtime Flow

1. `main.cpp` initializes path/config/log/crash report and calls `CSDOLApp::Initial(hInstance)`.
2. `CSDOLApp` creates/owns the main login process state, `CLoginModule2`, app info, and login dialog.
3. `CAuthenManager::Initialize()` reads `CConfigManager` and `CSDOLAppInfo`, calls `SdoBase_Initialize3(...)`, and registers callbacks for dynamic key, static login result, dynamic-login continuation, FCM, QR, push, logout, SMS, and other result types.
4. UI classes call `CAuthenManager::Async*` methods with `this` as user data.
5. `Authen.cpp` calls `SdoBase_SetUserData(handle, pUserData)` before each request.
6. SdoBase callbacks convert raw callback data into old structs such as `SGetDynamicKeyResult`, `SAuthenResult`, `SSendPushMessageResult`, `SGetCodeKeyResult`.
7. `CAuthenManager::NotifyLoginWnd(...)` posts a `WM_LOGIN_*` message to the current login window.
8. The Duilib window receives the message, checks `result.m_pUserData == this`, and updates UI or calls `SendLoginSuccessNotify`.

Qt replacement:

- Replace HWND message dispatch with Qt queued signals or a typed event bus.
- Keep user-data correlation semantics, because multiple dialogs can initiate auth requests.
- Keep SdoBase request ordering, error-code mapping, and callback payload fields.
- Keep SDK-facing result fields (`UserName`, `SessionId`, `Sndaid`, `Appendix`, `SecurityLevel`, double-login data) compatible.

## Password Login

Old entry: `LoginDlg.cpp`, `CLoginDlg::OnBtnOk`.

1. Button/Enter triggers `OnBtnOk`.
2. Plays click sound if `ShowLoingVoice=True`.
3. Requires protocol checkbox: `ContainerDlg::GetProtocolSelected()`.
4. Validates account and password:
   - If `GetSupportMobileAccountOnly()` is true, account must be non-empty and only digits or `+`.
   - Otherwise account and password must both be non-empty.
5. Calls `CheckAreaSelected()`.
6. Disables login button.
7. Calls `CSDOLApp::GetInstance()->UpdateAppInfo()`.
8. Calls `CAuthenManager::AsyncGetDynamicKey(this)`.
9. `Authen.cpp` calls `SdoBase_GetDynamicKey(handle)` and posts `WM_LOGIN_GET_DYNAMICKEY`.
10. `CLoginDlg::OnGetDynamicKeyResult` checks `m_pUserData == this`.
11. On dynamic-key failure, shows prompt and re-enables login button.
12. On success:
    - Reads encrypted password from old password edit using challenge code.
    - Trims account.
    - Reads game-account checkbox into `inputUserType`.
    - Calls `CAuthenManager::AsyncStaticLogin(trimmedUser, encryptedPassword, NULL, inputUserType)`.
13. `CAuthenManager::AsyncStaticLogin`:
    - Clears internal state.
    - Sets login type password.
    - Base64-decodes password.
    - Encrypts user and password with dynamic key using `sdoa_encrypt`.
    - Uses scene `pc_login` or `xp_login`.
    - Calls `SdoBase_StaticLoginWithGameAccount(handle, ename, epassword, 1, 0, 0, 2, inputUserType, 0, scene)`.
14. `onLoginResultCallback` creates `SAuthenResult` and posts `WM_LOGIN_AUTHEN_RESULT`.
15. `CLoginDlg::OnAuthenResult`:
    - Re-enables login button and clears password.
    - On success, records last user info, closes keyboard, calls `SendLoginSuccessNotify`.
    - On failure, shows error prompt unless special `-10386308`, then may trigger static-login-condition flow.

Qt implementation target:

- `PasswordLoginPage::submit()` should call `AuthService::requestDynamicKey(LoginFlow::Password, requestContext)`.
- Dynamic-key success should call `AuthService::passwordLogin(...)`, not mock success.
- Password encryption must match old edit control output. If old `GetPasswordText()` cannot be reused, implement a compatibility helper and verify against old behavior.
- Login result signal should route to common success/failure handler and SDK callback.

## One-Click Push Login

Old entry: `PushMessageLoginDlg.cpp`, `CPushMessageLoginDlg::OnBtnLogin`.

1. Initial UI is `push_message/UIDialog.xml`.
2. Important controls:
   - `AccountCombox`: account input.
   - `GetSerialNumberBtn`: initial "连接" action button.
   - `RetryBtn`: hidden style/template button for retry state.
   - `StatusHintText`: serial number display.
   - Status labels show install/version/online/switch/blacklist progress.
3. Click or Enter triggers `OnBtnLogin`.
4. Requires protocol checkbox.
5. Calls `CSDOLApp::UpdateAppInfo()`.
6. Kills polling timer.
7. Checks area selection.
8. Calls `CAuthenManager::AsyncGetDynamicKey(this)`.
9. Dynamic-key result arrives via `WM_LOGIN_GET_DYNAMICKEY`.
10. On success, old flow calls `AsyncSendPushMessage(strUserNameA, this)`.
11. `CAuthenManager::AsyncSendPushMessage`:
    - Saves current user.
    - Uses scene `pc_pushmsglogin` or `xp_pushmsglogin`.
    - Calls `SdoBase_SendPushMessage(handle, user, scene)`.
12. `onSendPushMessageCallback` posts `WM_LOGIN_GETPUSHMESSAGE_RESULT` with `SSendPushMessageResult`.
13. `OnSendPushMessageResult` success:
    - Starts `TimerEventID` polling every 1000 ms.
    - Saves current account.
    - Shows prompt "一键登录确认码已发送，请打开手机确认".
    - Shows serial number in `StatusHintText`.
    - Sets first two status labels to success.
    - Sets countdown to `6`.
    - Changes `GetSerialNumberBtn` to retry button visuals/text by copying from `RetryBtn`.
14. Polling timer calls `CAuthenManager::AsyncPushMessageLogin(user, this)`.
15. `AsyncPushMessageLogin` calls `SdoBase_PushMessageLogin(handle, 0, 0, 0)`.
16. Login callback posts `WM_LOGIN_AUTHEN_RESULT`.
17. `OnPushMessageAuthenResult`:
    - If result OK, kills timer, records last user, reports, calls `SendLoginSuccessNotify`.
    - If error `-10516808`, this means "not confirmed yet"; keep polling and do not show error.
    - If error `-10516809`, user cancelled on phone; stop countdown and hide hint host.
    - Other errors stop polling, show prompt, and re-enable button.

Qt implementation target:

- `PushMessageLoginPage` must not show SMS verification controls by default.
- Initial action button uses `GetSerialNumberBtn` assets from `push_message` skin reference.
- Click calls `AuthService::requestDynamicKey(LoginFlow::PushMessage, ctx)`, then `sendPushMessage`.
- Retry state copies `RetryBtn` assets and countdown behavior.
- Use a Qt polling timer for `pushMessageLogin` and handle `-10516808` as a non-terminal state.

## QR Code Login

Old entry: `CodeKeyLoginDlg.cpp`, `CCodeKeyLoginDlg::Show`, `StartCodeKeyLoginState`, `OnBtnRefresh`.

1. Entering the tab calls `Show()`.
2. Sets `InCodeKeyPage=true`.
3. If protocol selected:
   - Sets company id 0.
   - Shows related title UI.
   - Calls `CSDOLApp::UpdateAppInfo()`.
   - Sets auth login window handle.
   - Restores previous QR UI state.
   - Starts login timing.
   - If state is normal and flow is not active, sets `CodeKeyFlowActive=true` and calls `AsyncGetDynamicKey(this)`.
4. If protocol not selected, QR label is hidden/error state is shown; old code does not request a QR.
5. Refresh image/button calls `StartCodeKeyLoginState` or `OnBtnRefresh`:
   - Clears existing code key via `CAuthenManager::ClearCodeKey()`.
   - Calls `AsyncGetDynamicKey(this)`.
6. Dynamic-key callback posts `WM_LOGIN_GET_DYNAMICKEY`.
7. `OnGetDynamicKeyResult`:
   - On failure, shows prompt.
   - On success, sets request param `maxsize` to QR label width and calls `AsyncGetQrCode(this)`.
8. `AsyncGetQrCode` calls `SdoBase_GetQrCode(handle)`.
9. `onGetQrCodeCallback` posts `WM_LOGIN_GETCODEKEY_RESULT`.
10. `OnShowCodeKey`:
    - Converts returned binary image data through `IStream`/GDI+.
    - Sets image on `CodeKeyLabel`.
    - Starts `TimerEventID` polling every 1000 ms.
11. Polling timer sets `maxsize` again and calls `AsyncQrCodeLogin(this)`.
12. `AsyncQrCodeLogin` calls `SdoBase_QrCodeLogin(handle, 0, 0, 0)`.
13. `OnCodeKeyAuthenResult`:
    - OK: kill timer, clear flow active, reset QR state, report, `SendLoginSuccessNotify`.
    - Error `-10515805`: polling state. If `isScanned == "1"`, show scanned/wait-confirm overlay; otherwise keep normal QR.
    - Error `-10515801`: QR expired/failed. Kill timer, set failed overlay, show refresh.
    - Other errors: kill timer, show prompt.

Qt implementation target:

- `QrLoginPage` should only request QR after protocol is checked.
- Use `QPixmap::loadFromData(picData, picLength)` for the returned image.
- Maintain three states: normal, scanned, failed.
- Poll with `QTimer` and preserve non-terminal error handling.

## SMS Login And Captcha

Old entries: `SMSLoginDlg.*`, `CaptchaDlg.*`, `GoDownSMSLoginDlg.*`, `SMSLoginDlgVoice.*`.

Main behavior:

1. User enters phone and asks for code.
2. Flow may require captcha before sending SMS.
3. Captcha dialogs call one of:
   - `AsynCheckCodeToSendSms`
   - `AsyncGoDownSendSmsCheckCode`
   - `AsyncGuardDianSendSmsCheckCode`
4. Normal SMS login calls `AsyncSendSmsCheckCode`, then `AsyncGetDynamicKey`, then `AsyncSmsLogin`.
5. SdoBase login result returns through the same `onLoginResultCallback` and `WM_LOGIN_AUTHEN_RESULT` path.

Qt implementation target:

- Implement SMS as first-class Qt page/dialog after password/push/QR.
- Captcha should use the same external CEF browser process used by protocol/GeeTest.
- Keep owner-dialog routing so captcha completion resumes the dialog that opened it.

## Secondary Auth Continuations

Old callback: `SdoBase_DynamicLoginCallback` in `SdoBaseClient.h`; handled in `Authen.cpp` by creating `SAuthenContinue` and posting `WM_LOGIN_AUTHEN_CONTINUE`.

Old UI routing is in `LoginCommon.h`:

- Device type 3: `ProtectA8`.
- Device type 1/2 and related dynamic-password devices: `ProtectD6`.
- Card protection: `ProtectCardDlg`.
- Captcha/geetest: `ProtectCode` or `Geetest`.
- FCM/real-name and guardian info are separate messages/result structs.

Qt implementation target:

- `AuthService` emits `authContinuation(AuthContinuation)`.
- Main login controller opens the matching Qt dialog.
- The continuation dialog calls back into `AuthService` with `dynamicLogin`, `checkCodeLogin`, `checkCodeLoginNew`, `fcmLogin`, or guardian APIs.

## SDK And IPC Result Flow

Old success path:

1. UI calls `SendLoginSuccessNotify(SAuthenResult*)`.
2. This converts auth result to old SDK `SDOLLoginResult` fields:
   - `UserName`
   - `SessionId`
   - `Sndaid`
   - `Appendix`
   - `SecurityLevel`
3. `CSDOLApp` stores the login result.
4. `CLoginModule2` broadcasts login success through old `INMessage` and PB channels.
5. SDK caller obtains ticket/session through exported SDK interface.

Qt implementation target:

- Keep SDK ABI exports unchanged in `sdologinsdk.dll`.
- Keep `sdologin.exe` IPC commands/responses compatible.
- New auth result model must include all old fields even if UI only displays success/failure.
- DX8/DX9/DX11 embedding must still call the same SDK display-login interface; the child Qt login window must follow owner HWND.

## What To Implement Before More UI Polishing

1. `LegacyAuthService`: SdoBase initialization, dynamic key, password, push send/poll, QR get/poll, SMS send/login, logout, extend state.
2. `LegacyAuthResult` and `AuthContinuation` models matching old structs.
3. `LoginController`: maps Qt button events to old request order.
4. `LoginIpcServer`: preserves SDK process communication and result responses.
5. `DialogRouter`: opens secondary auth/compliance dialogs from auth signals.
6. Browser process integration: protocol, captcha/GeeTest, face/user-collect pages.
