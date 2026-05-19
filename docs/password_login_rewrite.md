# 密码登录 Qt 重写说明

## 旧工程入口

- 界面入口：`trunk/src/Login/LoginDlg.h`、`trunk/src/Login/LoginDlg.cpp`
- 认证入口：`trunk/src/Login/Authen.h`、`trunk/src/Login/Authen.cpp`
- 密码编辑框：`trunk/src/duilib/UISdoEdit.cpp`、`trunk/src/duilib/SDOUI/SdoEditWeakPwd.cpp`
- 软键盘：`trunk/src/Login/Keyboard.h`、`trunk/src/Login/Keyboard.cpp`
- SdoBase 接口：`trunk/include/SdoBaseClient.h`

## 旧密码登录链路

1. `CLoginDlg::OnBtnOk` 校验协议、账号、密码、区服选择。
2. 禁用登录按钮，并调用 `CAuthenManager::AsyncGetDynamicKey(this)`。
3. `SdoBase_GetDynamicKey` 回调 `onGetDynamicKeyCallback`，窗口收到 `OnGetDynamicKeyResult`。
4. `CSdoEditUI::GetPasswordText(dynamicKey)` 通过 `SafeStore`/`IAlgorithm::RSA` 生成旧密码密文，结果是 Base64 文本。
5. `CAuthenManager::AsyncStaticLogin` 对 Base64 密码做 Decode，随后用 `sdoa_encrypt` 分别加密账号和密码。
6. 调用 `SdoBase_StaticLoginWithGameAccount(handle, ename, epassword, 1, 0, 0, 2, inputUserType, keepLoginFlag, scene)`。
7. 成功回调 `SdoBase_LoginResultCallback`，失败或二次认证回调对应窗口消息。

## Qt 新实现

- `src/sdologin/legacy_password_auth.*`
  - 提供密码登录校验、账号 trim、旧 Base64 编解码、`StaticLoginWithGameAccount` 载荷组装。
- `src/sdologin/legacy_password_crypto.*`
  - Win32 下调用旧 `SafeStore.lib` 的 `IAlgorithm::RSA` 和 `sdoa_encrypt`。
  - x64 下不做假成功，直接返回明确错误。
- `src/sdologin/sdo_base_password_auth.*`
  - 轻量版 `CAuthenManager`，保留旧核心链路：初始化 SdoBase、获取动态密钥、密码加密、静密登录、登录结果回调。
- `src/sdologin/login_window.*`
  - 密码登录按钮已接入 `SdoBasePasswordAuthenticator`。
  - 登录成功后把 `sessionId/ticket/sndaid` 回传 SDK IPC。
- `src/sdologin/virtual_keyboard_model.*`
  - 按旧 `Keyboard.cpp` 的 47 个按键和行内随机规则建模。
- `src/sdologin/virtual_keyboard_dialog.*`
  - 使用 `skin/keyboard` 资源绘制 Qt 软键盘。

## 当前限制

- 当前机器 Qt kit 是 `Qt 6.5.3 msvc2019_64`，但旧 `trunk/lib/SdoBaseClient.lib`、`trunk/bin/*/SdoBaseClient.dll`、`trunk/lib/SafeStore.lib` 均为 32 位。
- 因此当前 x64 构建只能编译通过和展示 UI，真实密码登录会提示需要 Win32 构建。
- 要让真实服务端密码登录跑通，需要安装 32 位 Qt（建议 Qt 5.15.x msvc2019，兼顾 Win7）并用 VS2019 Win32 kit 生成工程；CMake 已在 Win32 下自动链接 `SdoBaseClient.lib`、`SafeStore.lib` 并复制 `SdoBaseClient.dll`。

## 下一步

- 接入二次认证窗口：图验、动态密码、密保、实名。
- 将一键登录、扫码、短信登录也迁移到同一套 `SdoBasePasswordAuthenticator` 风格的认证适配层。
- 如果后续提供 64 位 SdoBaseClient/SafeStore，则可以打开 x64 真实认证。
