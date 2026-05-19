# Legacy Login Module Code Map

> Scope: `trunk/src/Login` old `sdologin.exe` login module.  
> Purpose: describe each C++ source/header unit, decide what must be carried into the Qt rewrite, and avoid implementing Qt flows from guesses.

## Architecture Snapshot

The old login process is not only UI. It is a process-level module made of five layers:

1. Process bootstrap: `main.cpp`, `Application.*`, `SDOLApp.*`.
2. SDK and process IPC: `LoginModule.*`, `Message*. *`, PB message helpers, SDK/plugin module notifications.
3. Authentication adapter: `Authen.*`, which wraps `SdoBaseClient.h` and converts callbacks to window messages.
4. Duilib UI flow: `ContainerDlg.*`, `LoginDlg.*`, `PushMessageLoginDlg.*`, `CodeKeyLoginDlg.*`, `SMSLoginDlg.*`, plus modal dialogs.
5. Optional systems: third-party channels, payment, game update, promotion/ad, embedded IE/web widgets, in-game overlay windows.

For the Qt rewrite, the external SDK ABI can remain compatible while nearly all internal code should be rewritten. The exception is business behavior: SdoBase calls, config keys, callback result semantics, and IPC events must preserve the old contract.

## Migration Legend

| Mark | Meaning |
| --- | --- |
| Must rewrite now | Needed for password, one-click, QR, SMS/protocol, SDK callback, IPC, or configuration parity. |
| Rewrite as Qt dialog | Old Duilib dialog should become a Qt widget/dialog with same business behavior. |
| Adapter only | Keep the protocol/behavior but replace implementation with a narrow Qt/Win32 adapter. |
| Defer unless enabled | Feature is real, but can be implemented after the main login path unless config/channel requires it. |
| Do not port directly | Old infrastructure or bundled third-party source should be replaced or omitted. |

## Core Files

| Files | Role in old `sdologin.exe` | Qt rewrite decision |
| --- | --- | --- |
| `main.cpp` | WinMain entry. Initializes logging, paths, config, crash reporting, optional DPI behavior, launcher GUID, then starts `CSDOLApp` message loop. | Must rewrite now. Qt `sdologin.exe` should keep command parsing, DPI compatibility for Win7+, config initialization, crash/log setup, and process lifetime. |
| `Application.h`, `Application.cpp` | ATL/Duilib application bootstrap. Sets Duilib skin path/resource behavior and app-level initialization. | Adapter only. Replace Duilib setup with Qt resource/skin resolver. Preserve skin-type config interpretation. |
| `SDOLApp.h`, `SDOLApp.cpp` | Main process singleton. Owns app info, game window, login dialog, login result, IPC dispatch, browser/widget opening, face/pay/update hooks, ticket/session APIs, and final SDK notifications. | Must rewrite now, but split. New Qt project should not keep a huge singleton; create focused `AppRuntime`, `LoginIpcServer`, `AuthService`, `BrowserService`, and `LoginResultStore`. |
| `LoginModule.h`, `LoginModule.cpp` | IPC node for SDK/plugin process communication. Receives `INMessage` and PB messages, sends login launched/success/logout/user info/ticket/auth-code/browser/pay events. | Must rewrite now. Preserve external message IDs and response payloads. Internally use a new IPC layer instead of old `CProcNode` where possible. |
| `LoginInterface.h` | Interface abstraction for login dialog operations. | Must rewrite now as a Qt-facing interface between SDK command handling and UI. |
| `Message.h` | Defines old `WM_LOGIN_*`, `WM_APPMSG_*` custom messages and request structs such as `OpenBrowserMsg`. | Must rewrite now as enum/event definitions. Do not rely on HWND messages internally, but keep mapping for compatibility. |
| `StdAfx.h`, `StdAfx.cpp` | VS2008 precompiled header and common legacy includes. | Do not port directly. Replace with CMake/VS2019 normal includes. |
| `resource.h` | Win32 resource IDs. | Defer unless the new exe still needs legacy resources/version icons. |

## Authentication And Config

| Files | Role in old code | Qt rewrite decision |
| --- | --- | --- |
| `Authen.h`, `Authen.cpp` | Central adapter to `SdoBaseClient.h`. Defines result structs and calls `SdoBase_Initialize3`, `SdoBase_GetDynamicKey`, `SdoBase_StaticLoginWithGameAccount`, `SdoBase_SendPushMessage`, `SdoBase_PushMessageLogin`, `SdoBase_GetQrCode`, `SdoBase_QrCodeLogin`, SMS, captcha, FCM, guardian, face, payment ticket, logout, extend state. Converts callbacks to `WM_LOGIN_*`. | Must rewrite now. Create a Qt `LegacyAuthService` with signals instead of HWND posts. Keep SdoBase request order, scenes, error-code mapping, callback payload fields, and `SdoBase_SetUserData` correlation. |
| `ConfigManager.h`, `ConfigManager.cpp` | Loads encrypted XML config/user config/game config; exposes hundreds of feature flags, URLs, app IDs, protocol versions, skin/channel settings, login state flags. | Must rewrite now in smaller modules. Start with keys required by current login: passport server URLs, app info, protocol URLs/versions, support-mobile-only, skin type, browser mode, QR/push flags, InitSwitch, DebugMode, CEF/browser settings. |
| `ErrorMessage.h`, `ErrorMessage.cpp` | Maps fixed error codes to user-facing messages and fallback text. | Must rewrite now for dialogs/error display. |
| `DataReportMaker.h`, `DataReportMaker.cpp` | Builds auth/login/interface timing reports and action-result telemetry. | Adapter only for phase 1; keep no-op or minimal compatible reporting first, then wire HTTP reporting. |
| `TimeRecorder.h`, `TimeRecorder.cpp` | Named timing spans for login/report events. | Adapter only. Replace with a small timer utility used by `AuthService` and reporting. |
| `HttpReportHelper.h`, `HttpReportHelper.cpp` | Async HTTP reporting helper. | Defer unless reports are required by acceptance. |
| `ComputerInfo.h`, `ComputerInfo.cpp` | Hardware/device fingerprint collection. | Adapter only. Needed if SdoBase/reporting/risk config consumes the same IDs. |
| `LauncherInfoManager.h`, `LauncherInfoManager.cpp` | Launcher metadata and GUID handling. | Must rewrite now if launcher GUID/config affects SdoBase initialization or reports. |
| `RiskControlManager.h`, `RiskControlManager.cpp` | Risk-control helpers. | Defer unless config enables it in target games. |
| `ReportDeviceHelper.h`, `ReportDeviceHelper.cpp` | Device-report helper. | Defer unless required by reporting/risk-control acceptance. |

## Main Login UI

| Files | Role in old code | Qt rewrite decision |
| --- | --- | --- |
| `ContainerDlg.h`, `ContainerDlg.cpp` | Main Duilib container. Loads `container/UIDialog*.xml`, owns tab layout, protocol checkbox, close/minimize, area selection, third-party buttons, tab switching, child login pages, SDK close behavior. | Must rewrite now. Qt main login window should mirror this state machine, but not Duilib. |
| `LoginCommon.h` | Template/common base for login tab pages and secondary auth dialog dispatch. Sends login success notification into parent/SDK result. | Must rewrite now as shared Qt controller helpers. |
| `LoginDlg.h`, `LoginDlg.cpp` | Password login tab. Validates protocol/account/password/area, requests dynamic key, encrypts password with old password edit challenge behavior, calls static login, handles auth result and secondary auth. | Must rewrite now. This is the reference flow for Qt password login. |
| `PushMessageLoginDlg.h`, `PushMessageLoginDlg.cpp` | One-click phone push login tab. Uses `push_message/UIDialog.xml`, gets dynamic key, sends push message, shows serial number, starts polling `AsyncPushMessageLogin`, changes login button to retry style. | Must rewrite now. Qt one-click page must follow this flow, including initial button from `GetSerialNumberBtn` and retry assets from `RetryBtn`. |
| `CodeKeyLoginDlg.h`, `CodeKeyLoginDlg.cpp` | QR code login tab. Gets dynamic key, requests QR image binary, renders image, polls `AsyncQrCodeLogin`, tracks normal/scanned/failed states. | Must rewrite now. Qt should display returned image data directly in `QPixmap`, then poll via `QTimer`. |
| `SMSLoginDlg.h`, `SMSLoginDlg.cpp` | SMS login tab: send SMS code, optional captcha, dynamic key, SMS login, timers. | Rewrite as Qt dialog/page. Needed because user requested SMS flow parity. |
| `SecureUnisignonDlg.h`, `SecureUnisignonDlg.cpp` | Saved account/fast login list. Loads `accountlst/UIDialog.xml`, clears QR state, requests fast login/check-account-login-type. | Rewrite as Qt dialog/page if auto-login/saved-account is enabled. |
| `PhoneRegisterDlg.h`, `PhoneRegisterDlg.cpp` | Phone registration/login assist flow: account type check, dynamic key, send phone code, phone-code login. | Rewrite as Qt dialog/page after core login. |
| `StaticLoginDlg.h`, `StaticLoginDlg.cpp` | Static-login-condition prompt shown for special error `-10386308`. | Rewrite as Qt dialog. |
| `SecurityPhoneDlg.h`, `SecurityPhoneDlg.cpp` | Security phone verification dialog. | Rewrite as Qt dialog if returned by SdoBase flow. |
| `SecurityPhoneStaticLoginDlg.h`, `SecurityPhoneStaticLoginDlg.cpp` | Security phone dialog variation for static login condition. | Rewrite as Qt dialog if this flow is kept. |
| `ActivateLoginDlg.h`, `ActivateLoginDlg.cpp` | Active code login dialog. Calls `AsyncActiveCodeLogin` and returns auth result. | Rewrite as Qt dialog. Triggered by `WM_LOGIN_SHOW_ACTIVE_CODE_LOGIN_DLG`. |
| `CaptchaDlg.h`, `CaptchaDlg.cpp` | Captcha dialog used before sending SMS/guardian/go-down SMS. Calls captcha-to-send-SMS auth APIs and returns to owner window. | Rewrite as Qt dialog. |
| `GeetestDlg.h`, `GeetestDlg.cpp` | GeeTest/H5 captcha holder and static type state. | Rewrite with CEF/browser process or Qt WebEngine/CEF OSR depending final browser architecture. |

## Secondary Auth And Compliance Dialogs

| Files | Role in old code | Qt rewrite decision |
| --- | --- | --- |
| `ProtectA8.h`, `ProtectA8.cpp` | A8 dynamic-password secondary auth dialog. Calls `AsyncDynamicLogin`. | Rewrite as Qt dialog. |
| `ProtectD6.h`, `ProtectD6.cpp` | D6/X6/X8/E8 coordinate/key secondary auth dialog. Calls `AsyncDynamicLogin`. | Rewrite as Qt dialog. |
| `ProtectCardDlg.h`, `ProtectCardDlg.cpp` | Security card secondary auth. Calls `AsyncDynamicLogin`. | Rewrite as Qt dialog. |
| `ProtectCode.h`, `ProtectCode.cpp` | Image captcha login continuation. Calls `AsyncCheckCodeLogin`. | Rewrite as Qt dialog. |
| `CGamePlayerInfoDlg.h`, `CGamePlayerInfoDlg.cpp` | Anti-addiction/real-name info collection. Calls `AsyncUpdateFcmInfo`, UE init/report, optional close confirm. | Rewrite as Qt dialog. |
| `CGamePlayerTwiceConfirmInfoDlg.h`, `CGamePlayerTwiceConfirmInfoDlg.cpp` | Real-name second confirmation dialog. | Rewrite as Qt dialog. |
| `CGuardianPlayerInfoDlg.h`, `CGuardianPlayerInfoDlg.cpp` | Guardian/minor info collection with SMS/protocol controls. | Rewrite as Qt dialog. |
| `CGuardianConfirmPlayerInfoDlg.h`, `CGuardianConfirmPlayerInfoDlg.cpp` | Guardian info submit confirmation. Calls `AsyncGuardDianSubmitConfirmMessageResult`. | Rewrite as Qt dialog. |
| `CGuardDianCaptchaDlg.h`, `CGuardDianCaptchaDlg.cpp` | Guardian SMS captcha. Calls `AsyncGuardDianSendSmsCheckCode`. | Rewrite as Qt dialog. |
| `CGuardianDlgVoice.h`, `CGuardianDlgVoice.cpp` | Guardian voice verification. Calls `AsyncGuardDianSendSms`. | Rewrite as Qt dialog. |
| `SMSLoginDlgVoice.h`, `SMSLoginDlgVoice.cpp` | Voice SMS verification for go-down/SMS flows. | Rewrite as Qt dialog if voice option remains. |
| `GoDownSMSLoginDlg.h`, `GoDownSMSLoginDlg.cpp` | "Go down" account confirmation/SMS fallback flow. Calls relative-account, send-SMS, confirm-login APIs. | Rewrite as Qt dialog after password/SMS basics. |
| `GoDownCaptchaDlg.h`, `GoDownCaptchaDlg.cpp` | Captcha sub-flow for go-down SMS send. | Rewrite as Qt dialog. |
| `FaceCodeDlg.h`, `FaceCodeDlg.cpp` | Face verification QR/action polling. Calls face action/result/init APIs. | Rewrite as Qt dialog/page if face verification is returned. |
| `FaceVertifyIeDlg.h`, `FaceVertifyIeDlg.cpp` | Embedded IE page for face verification and query ticket status. | Replace with browser-process CEF window/OSR page. |
| `FaceCollectUserIeDlg.h`, `FaceCollectUserIeDlg.cpp` | Embedded IE page for collecting face-holder user info. | Replace with browser-process CEF window/OSR page. |
| `CFaceVerifyDlgCloseDlg.h`, `CFaceVerifyDlgCloseDlg.cpp` | Close-confirm dialog for face verification page. | Rewrite as Qt dialog. |
| `CFaceCollectMsgDlgCloseDlg.h`, `CFaceCollectMsgDlgCloseDlg.cpp` | Close-confirm dialog for face-collect page. | Rewrite as Qt dialog. |
| `FaceVerifyPromptDlg.h`, `FaceVerifyPromptDlg.cpp` | Small face verification prompt/helper. | Rewrite as Qt dialog if referenced by face flow. |

## Protocol, Browser, And Web Bridge

| Files | Role in old code | Qt rewrite decision |
| --- | --- | --- |
| `ProtocolDlg.h`, `ProtocolDlg.cpp` | Protocol display dialog using `protocol/UIDialog.xml`. | Must rewrite now. Use Qt dialog and external browser process/CEF OSR for protocol content. |
| `AgreementDlg.h`, `AgreementDlg.cpp` | Agreement IE dialog using `agreementie/UIDialog.xml`; close may close login. | Replace with Qt + browser process. |
| `IeDlg.h`, `IeDlg.cpp` | Generic embedded IE dialog base. | Do not port directly. Replace with `qtlogin_browser` process and CEF 109. |
| `BusinessHelper.h`, `BusinessHelper.cpp` | Parses special URL commands, opens widget/browser, requests client VKey for protected URL. | Adapter only. Keep URL command behavior; replace window opening with browser service. |
| `WidgetManager.h`, `WidgetManager.cpp` | Loads `config/widget.xml`, opens web widgets, injects SSO cookie/session info, opens face/pay widgets. | Adapter only. Split into `WidgetConfig` and `BrowserService`. |
| `WebProxy.h`, `WebProxy.cpp` | COM/ROT bridge to web UI component. | Do not port directly unless old web component requires COM bridge. |
| `WebReceiver.h`, `WebReceiver.cpp` | Web expand receiver/factory, handles web-to-login messages. | Adapter only if current CEF pages need this callback contract. |
| `PayMentDlg.h`, `PayMentDlg.cpp` | Payment browser dialog. | Defer unless payment is in acceptance; replace IE with browser process. |
| `PromotionDlg.h`, `PromotionDlg.cpp` | Promotion/ad IE dialog using downloaded XML/resource zip. | Defer unless promotion required. |
| `XinYouLoginDlg.h`, `XinYouLoginDlg.cpp` | XinYou browser login window, timer and close handling. | Defer unless XinYou channel required; replace IE. |
| `ShunWangLoginDlg.h`, `ShunWangLoginDlg.cpp` | ShunWang IE login window. | Defer unless ShunWang channel required; replace IE. |
| `FeiHuoLoginDlg.h`, `FeiHuoLoginDlg.cpp` | Thin FeiHuo IE login wrapper. | Defer unless FeiHuo channel required. |
| `FeihuoDlg.h`, `FeihuoDlg.cpp` | FeiHuo third-party login flow: dynamic key, callback, third-party login. | Defer unless FeiHuo channel required. |

## Messaging And Notifications

| Files | Role in old code | Qt rewrite decision |
| --- | --- | --- |
| `MessageAdapter.h`, `MessageAdapter.cpp` | Old game-update/download message structs and adapter types. | Defer unless update module is kept. |
| `MessageFactory.h`, `MessageFactory.cpp` | Creates typed `INMessage` objects from IPC bytes. | Must rewrite now for SDK IPC compatibility, but can be narrower for login-related messages first. |
| `MessageServiceAdapter.h`, `MessageServiceAdapter.cpp` | Message-service payload model for image/text notification content. | Defer unless login-notify/promotion service required. |
| `MessageServiceManager.h`, `MessageServiceManager.cpp` | Message-service manager for notification data. | Defer unless login-notify required. |
| `LoginNotifyDlg.h`, `LoginNotifyDlg.cpp` | Post-login notification dialog, account info/history collection, message-service view. | Defer unless old SDK callers expect login notify UI. |
| `LoginPromptDlg.h`, `LoginPromptDlg.cpp` | Floating prompt UI. | Rewrite as Qt lightweight prompt if used by current UI. |
| `LoginStateManager.h`, `LoginStateManager.cpp` | Tracks game/login state for SDK module PID, app/area/game window, user and account state messages. | Adapter only. Needed if SDK callers use login-state query/extend-state behavior. |
| `Observable.h`, `Observable.cpp`, `Observer.h` | Simple observer pattern used by common windows/dialogs. | Do not port directly. Qt signals/slots replace it. |

## Windowing And In-Game Overlay Infrastructure

| Files | Role in old code | Qt rewrite decision |
| --- | --- | --- |
| `XWindow.h`, `XWindow.cpp` | Base Duilib/Win32 window wrapper. Supports inside-game/outside-game modes, owner game window, topmost/toolwindow, combo drop windows. | Do not port directly. Preserve behavior in Qt embedding/window-style helpers. |
| `CommonWindow2.h`, `CommonWindow2.cpp` | Common modal dialog base with prompt helpers and observer behavior. | Do not port directly. Replace with `QtDialogBase` and shared prompt service. |
| `OGWindow.h`, `OGWindow.cpp` | Outside-game window variant. | Replace with Qt top-level/child mode handling. |
| `IGWindow.h`, `IGWindow.cpp` | Inside-game/injected window base with shared image writer and relation to game window. | Adapter only. Required conceptually for DX embed, but implementation should be rewritten. |
| `IGAOutGameModule.h`, `IGAOutGameModule.cpp` | Out-game module message handling for inside-game assistant. | Defer unless full IGA module needed. |
| `IGWindowManager.h`, `IGWindowManager.cpp` | Manages in-game overlay windows, cursor, z-order. | Defer after login embed works. |
| `IGMessageDispatcher.h`, `IGMessageDispatcher.cpp` | Dispatches in-game window/input messages. | Defer unless old in-game overlay feature required. |
| `IGComboWnd.h`, `IGComboWnd.cpp` | Combo dropdown window for in-game UI. | Replace with Qt combo behavior. |
| `IGCursor.h`, `IGCursor.cpp` | Duilib cursor window using `cursor/UIDialog.xml`. | Defer/replace. |
| `IGImeManager.h`, `IGImeManager.cpp` | IME support window using `ime/UIDialog.xml`. | Defer. Qt input method may replace it. |
| `IGTooltipWnd.h`, `IGTooltipWnd.cpp` | Tooltip window using `tooltip/UIDialog.xml`. | Replace with Qt tooltip. |
| `AccountListContainerElementUI.h` | Custom Duilib account-list row with remove button, `login/UIAccount.xml`. | Rewrite as Qt list item/delegate. |
| `Keyboard.h`, `Keyboard.cpp` | Soft keyboard dialog using `keyboard/UIDialog.xml`. | Rewrite as Qt virtual keyboard only if password UI keeps it. |
| `OpenAnimationDlg.h`, `OpenAnimationDlg.cpp` | Opening animation dialog. | Defer; not needed for functional parity. |

## Game Update And Plugin

| Files | Role in old code | Qt rewrite decision |
| --- | --- | --- |
| `GameUpdate.h` | Placeholder/empty header. | Do not port. |
| `GameUpdateDlg.h`, `GameUpdateDlg.cpp` | ActiveX-based game update UI using `gameupdate/UIDialog.xml`. | Defer unless updater must be included in Qt rewrite. |
| `GameUpdateFactory.h`, `GameUpdateFactory.cpp` | Factory for game update objects. | Defer. |
| `SdgPatchAdapter.h`, `SdgPatchAdapter.cpp` | SDG patch/update adapter. | Defer. |
| `SdgUpdate.h`, `SdgUpdate.cpp` | SDG updater facade. | Defer. |
| `SqexPatchAdapter.h`, `SqexPatchAdapter.cpp` | SQEX patch/update adapter. | Defer. |
| `SqexUpdate.h`, `SqexUpdate.cpp` | SQEX updater facade. | Defer. |
| `GameSettingDlg.h`, `GameSettingDlg.cpp` | Game settings dialog using `gamesetting/UIDialog.xml`. | Defer. |
| `GamePlugin.h` | Game plugin manager/multisingleton integration. | Adapter only if SDK IPC still sends plugin events. |
| `PluginModule.h`, `PluginModule.cpp` | Plugin module IPC/node wrapper. | Defer unless plugin IPC remains in acceptance. |
| `KitManager.h`, `KitManager.cpp` | Kit/component manager. | Defer. |
| `ProcessMonitor.h`, `ProcessMonitor.cpp` | Monitors/deletes processes. | Defer unless update/plugin needs it. |

## Third-Party Channel And Payment

| Files | Role in old code | Qt rewrite decision |
| --- | --- | --- |
| `ThirdLoginDlg.h`, `ThirdLoginDlg.cpp` | Third-party login tab. Chooses skin by `skintype`, shows Lenovo/ShunWang/MiGu/FeiHuo/XinYou buttons. | Defer unless target config enables third-party tab. |
| `ThirdLoginWindow.h`, `ThirdLoginWindow.cpp` | Transparent third-party polling/login window. Gets dynamic key then calls third-party login. | Defer. |
| `ThirdLoginImpl.h`, `ThirdLoginImpl.cpp` | Third-party company metadata and simple implementation helpers. | Defer. |
| `ThridLoginClient.h`, `ThridLoginClient.cpp` | Loader for `IThirdClient`. | Defer. |
| `MiGuLoginDlg.h`, `MiGuLoginDlg.cpp` | MiGu login dialog and SMS flow. | Defer unless MiGu required. |
| `LenovoDll.h` | Lenovo external DLL declarations. | Defer. |
| `LenovoHelper.h`, `LenovoHelper.cpp` | Lenovo callback helper, calls third login. | Defer. |
| `lenovo/LenovoLoginDlg.h`, `lenovo/LenovoLoginDlg.cpp` | Lenovo channel login. | Defer. |
| `lenovo/LenovoPay.h`, `lenovo/LenovoPay.cpp` | Lenovo payment order flow via ticket and Lx order. | Defer. |
| `lenovo/LenovoPayWindow.h`, `lenovo/LenovoPayWindow.cpp` | Lenovo payment browser/window wrapper. | Defer. |
| `DragonLenovo/DragonLenovoLoginDlg.h`, `DragonLenovo/DragonLenovoLoginDlg.cpp` | Dragon Lenovo channel login. | Defer. |
| `DragonLenovo/DragonLenovoPay.h`, `DragonLenovo/DragonLenovoPay.cpp` | Dragon Lenovo payment order flow. | Defer. |
| `DragonLenovo/DragonLenovoPayWindow.h`, `DragonLenovo/DragonLenovoPayWindow.cpp` | Dragon Lenovo payment window. | Defer. |
| `Matrix/360LoginDlg.h`, `Matrix/360LoginDlg.cpp` | 360/Matrix login dialog using `360_login/UIDialog.xml`. | Defer unless Matrix/360 channel required. |
| `Matrix/MatrixLoginDlg.h`, `Matrix/MatrixLoginDlg.cpp` | Matrix SDK login/pay wrapper; calls `AsyncThirdLogin`. | Defer. |
| `Matrix/MatrixHelper.h` | Matrix SDK helper declaration. | Defer. |
| `Matrix/MatrixSDKUtil.h` | Matrix SDK utility declarations. | Defer. |
| `Matrix/api/Define.h` | Matrix SDK API definitions. | Do not port directly unless Matrix SDK is enabled. |
| `Matrix/api/gameinfo.h` | Matrix SDK game info API. | Do not port directly unless Matrix SDK is enabled. |
| `Matrix/api/Perf.h` | Matrix SDK performance API. | Do not port directly unless Matrix SDK is enabled. |
| `Matrix/api/SDK.h` | Matrix SDK core COM-like API. | Do not port directly unless Matrix SDK is enabled. |
| `Matrix/api/user.h` | Matrix SDK user/web user API. | Do not port directly unless Matrix SDK is enabled. |
| `Matrix/api/WinDefine.h` | Matrix SDK Windows compatibility definitions. | Do not port directly unless Matrix SDK is enabled. |
| `Westone/ads_common_c.h` | Westone ADS C API common definitions. | Defer/third-party dependency. |
| `Westone/ads_sdk_global.h` | Westone ADS export/global macros. | Defer/third-party dependency. |
| `Westone/wg_ads_sdk_c.h` | Westone ADS SDK C declarations. | Defer/third-party dependency. |
| `Westone/WestoneEncryptObject.h`, `Westone/WestoneEncryptObject.cpp` | Westone encryption object wrapper. | Defer unless this channel is enabled. |

## Area, UI Extras, And Miscellaneous

| Files | Role in old code | Qt rewrite decision |
| --- | --- | --- |
| `AreaTypeSwitch.h`, `AreaTypeSwitch.cpp` | Area/server-type switch UI using `areatype_switch/UIDialog*.xml`. | Must rewrite if games require area selection before login. |
| `DoubleLoginConfirm.h`, `DoubleLoginConfirm.cpp` | 2P/double-login confirmation dialog. | Defer unless double-login config is enabled. |
| `DoubleLoginConfirmMessages.h`, `DoubleLoginConfirmMessages.cpp` | 2P login message dialog. | Defer. |
| `DoubleLoginConfirmTips.h`, `DoubleLoginConfirmTips.cpp` | 2P login timeout/tips dialog. | Defer. |
| `PlaySound.h`, `PlaySound.cpp` | Button click sound manager. | Defer. |
| `AdCoreManager.h`, `AdCoreManager.cpp` | Ad-core manager. | Defer. |
| `AdCoreUtil.h`, `AdCoreUtil.cpp` | Ad utility functions and timestamps. | Defer. |
| `BusinessHelper.h`, `BusinessHelper.cpp` | URL/action helper; listed above under browser but also used by UI clicks. | Adapter only. |
| `CLoginPromptDlg.h`, `CLoginPromptDlg.cpp` | See `LoginPromptDlg.*`; floating prompt. | Rewrite if used by Qt main window. |
| `json_reader.cpp`, `json_value.cpp`, `json_writer.cpp` | Bundled JsonCpp implementation. | Do not port directly. Use Qt JSON or a package already in the project. |

## Required First Migration Set

These should be brought into the Qt rewrite first, as new code with old behavior:

1. `main.cpp`, `SDOLApp.*`, `LoginModule.*`, `Message.h`, `MessageFactory.*`: process boot and SDK/IPC.
2. `ConfigManager.*`, `Authen.*`, `ErrorMessage.*`, `TimeRecorder.*`: config and SdoBase-facing auth adapter.
3. `ContainerDlg.*`, `LoginDlg.*`, `PushMessageLoginDlg.*`, `CodeKeyLoginDlg.*`, `SMSLoginDlg.*`, `LoginCommon.h`: main login UI and flows.
4. `ProtocolDlg.*`, `AgreementDlg.*`, `IeDlg.*`, `BusinessHelper.*`, `WidgetManager.*`: protocol/browser behavior, rewritten around the external CEF browser process.
5. `ProtectA8.*`, `ProtectD6.*`, `ProtectCardDlg.*`, `ProtectCode.*`, `CaptchaDlg.*`, `GeetestDlg.*`: secondary auth returned by SdoBase.
6. `CGamePlayerInfoDlg.*`, guardian dialogs, face dialogs, `ActivateLoginDlg.*`: compliance and special flows that can appear after a login result callback.

Everything under update/payment/third-party channel can be staged after password, one-click, QR, SMS, protocol, captcha, and compliance flows are working.
