# 密码登录运行链路补充

日期：2026-05-16

## 老工程链路结论

`trunk/src/Login/LoginDlg.cpp` 中点击密码登录后，流程不是 `getguid` 成功就结束，而是继续走静密登录：

1. `CLoginDlg::OnBtnOk` 校验协议、账号、密码和区服。
2. 调用 `CAuthenManager::AsyncGetDynamicKey(this)`。
3. `SdoBase_GetDynamicKey` 回调 `onGetDynamicKeyCallback`，回调参数同时包含 `dynamicKey` 和 `guid`。
4. `LoginDlg::OnGetDynamicKeyResult` 使用 `dynamicKey` 调 `CSdoEditUI::GetPasswordText(dynamicKey)`，生成旧 SDK 要求的密码 Base64 文本。
5. `CAuthenManager::AsyncStaticLogin` 对 Base64 密码解码，然后用 `sdoa_encrypt` 分别加密账号和密码。
6. 调用 `SdoBase_StaticLoginWithGameAccount(handle, ename, epassword, 1, 0, 0, 2, inputUserType, 0, scene)`。
7. 成功进入 `SdoBase_LoginResultCallback`；需要图验进入 `SdoBase_CheckCodeLoginCallback`；需要二次认证进入 `SdoBase_DynamicLoginCallback`。

这里 `guid` 只是动态密钥请求阶段返回的辅助字段，真正给密码加密链路使用的是 `dynamicKey`。如果只拿到 `guid` 但 `dynamicKey` 为空，静密登录不能继续。

## Qt 当前实现

本次对 `qtlogin_rewrite/src/sdologin/sdo_base_password_auth.cpp` 做了这些对齐：

- `SdoBase_Initialize3` 第一个登录验证码回调改为 `onCheckCodeLoginCallback`，不再传空。
- 初始化成功后显式调用 `SdoBase_SetCheckCodeLoginCallback`、`SdoBase_SetDynamicLoginCallback`、`SdoBase_SetLoginResultCallback`、`SdoBase_SetGetDynamicKeyCallback`。
- `onGetDynamicKeyCallback` 把 `guid` 继续传入 `handleDynamicKey`，并记录 `dynamicKeyLen/guidLen`。
- `handleDynamicKey` 在 `dynamicKey` 有效后继续走 `SafeStore` RSA 旧密码文本、Base64 解码、`sdoa_encrypt` 账号密码、`SdoBase_StaticLoginWithGameAccount`。
- 图验回调会返回 `needsCaptcha=true`、`challengeUrl`、`challengeWidth`、`challengeHeight`，后续可以直接接旧图验窗口或 Qt 图验窗口。
- 二次认证回调继续返回 `needsSecondFactor=true`，后续接密保、实名、手机等窗口。
- 登录结果回调保持 `sndaId/ticket/tgt/inputUserId` 回填到 SDK IPC 结果。

本次还把老工程里反复出现的错误码映射抽到 `mapLegacySdoBaseResultCode`：认证组件 `[-10130200, -10130100]` 会减去 `394000`，映射到登录器原来的 `[-10524200, -10524100]` 区间。

## 调试日志

运行目录下 `qtlogin_rewrite.log` 会出现 `password-auth` 组件日志，例如：

```text
password login begin: request dynamic key
SdoBase_GetDynamicKey returned 0
dynamic-key callback result=0 dynamicKeyLen=... guidLen=...
call SdoBase_StaticLoginWithGameAccount inputUserType=... keepLoginFlag=... scene=...
SdoBase_StaticLoginWithGameAccount returned 0
login-result callback result=...
```

日志只写阶段、返回码和字段长度，不记录账号明文、密码、`dynamicKey`、`ticket`。

## DPI 调整

`qtlogin_rewrite/src/sdologin/qt_dpi_config.cpp` 原来强制：

```text
QT_ENABLE_HIGHDPI_SCALING=0
QT_SCALE_FACTOR=1
QT_FONT_DPI=96
AA_DisableHighDpiScaling
AA_Use96Dpi
```

这会导致高 DPI 屏幕下 Qt 窗口像没有适配。本次改为：

- Qt 5.15 启用 `AA_EnableHighDpiScaling`。
- 启用 `AA_UseHighDpiPixmaps`。
- 设置 `QT_AUTO_SCREEN_SCALE_FACTOR=1` 和 `QT_ENABLE_HIGHDPI_SCALING=1`。
- 不再强制 `QT_SCALE_FACTOR=1` 和 `QT_FONT_DPI=96`。
- Windows 优先使用 Per Monitor DPI Awareness V2；系统不支持时降级到 System DPI aware / `SetProcessDPIAware`，兼容 Win7 及以上。
