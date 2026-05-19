# 登录流程文档 2026-05-18

## 总体结构

Qt 重写版 `sdologin.exe` 按登录方式拆分，`LoginWindow` 只负责主窗口、切页、协议入口和最终 SDK 回调。具体登录流程由独立模块负责：

- 静密登录：`legacy_password_auth.*` + `sdo_base_password_auth.*`
- 扫码登录：`code_key_login_model.*` + `sdo_base_code_key_auth.*` + `code_key_login_panel.*`
- 一键登录：`push_message_login_model.*` + `sdo_base_push_message_auth.*` + `push_message_login_panel.*`

旧 SDK 调用方仍然通过 `sdologinsdk.dll` 发起 `ShowLoginDialog`。SDK 启动 `sdologin.exe` 后，登录成功会通过 IPC 返回 `sessionId/sndaid/ticket/identityState`，再由 SDK 回调给调用方。

## 静密登录

参考旧工程：

- `trunk/src/Login/LoginDlg.h`
- `trunk/src/Login/LoginDlg.cpp`
- `trunk/src/Login/Authen.h`
- `trunk/src/Login/Authen.cpp`

Qt 当前流程：

1. 用户点击“密码登录”。
2. `LoginWindow::submitPasswordLogin` 校验协议、账号、密码。
3. `SdoBasePasswordAuthenticator::login` 调用 `SdoBase_GetDynamicKey`。
4. 动态密钥回调成功后，用旧 SDK 的加密规则生成静密登录参数。
5. 调用 `SdoBase_StaticLoginWithGameAccount`。
6. `SdoBase_LoginResultCallback` 返回后填充 `PasswordAuthResult`。
7. `LoginWindow::completePasswordLogin` 转换为 `LoginWindowResult` 并关闭登录器。

当前已保留的关键点：

- 动态密钥获取失败直接终止。
- 账号、密码沿用旧静密加密桥。
- 需要图验或二次认证时先返回提示，后续再接具体弹窗。

## 扫码登录

参考旧工程：

- `trunk/src/Login/CodeKeyLoginDlg.h`
- `trunk/src/Login/CodeKeyLoginDlg.cpp`
- `trunk/src/Login/Authen.h`
- `trunk/src/Login/Authen.cpp`

Qt 当前流程：

1. 进入扫码页，如果协议已勾选，`CodeKeyLoginPanel::activate` 自动开始拉码。
2. `SdoBaseCodeKeyAuthenticator::requestCodeImage` 调用 `SdoBase_GetDynamicKey`。
3. 动态密钥回调成功后设置 `maxsize`，调用 `SdoBase_GetQrCode`。
4. 二维码图片回调成功后展示图片，并启动 1 秒轮询。
5. 轮询调用 `SdoBase_QrCodeLogin(handle, 0, 0, 0)`。
6. 返回 `-10515805` 表示继续轮询；其中 `isScanned == "1"` 表示手机端已扫码但未确认。
7. 返回 `-10515801` 表示二维码失效，显示刷新态。
8. 返回 `0` 表示登录成功，回传 `sndaId/ticket/tgt/inputUserId`。

当前已增加：

- `ClickInterval` 配置限制“刷新二维码”按钮连续点击，默认 `1500` 毫秒。
- “需安装叨鱼”和“了解二维码登录详情”从配置打开外部浏览器。

## 一键登录

参考旧工程：

- `trunk/src/Login/PushMessageLoginDlg.h`
- `trunk/src/Login/PushMessageLoginDlg.cpp`
- `trunk/src/Login/Authen.h`
- `trunk/src/Login/Authen.cpp`

旧工程主流程：

1. 用户点击一键登录的“连接”按钮。
2. `CPushMessageLoginDlg::OnBtnLogin` 检查协议、区服选择，然后调用 `CAuthenManager::AsyncGetDynamicKey`。
3. `OnGetDynamicKeyResult` 校验动态密钥和账号。
4. 账号通过后先调用 `AsyncCacelPushMessageLogin`，清理上一轮一键登录状态。
5. `OnCancelSendPushMessageResult` 调用 `AsyncSendPushMessage`。
6. `AsyncSendPushMessage` 内部调用 `SdoBase_SendPushMessage(handle, account, scene)`，`scene` 在 XP 使用 `xp_pushmsglogin`，其他系统使用 `pc_pushmsglogin`。
7. 发送成功后显示服务端返回的确认码 `strSerialNumber`，启动 1 秒轮询。
8. 轮询调用 `SdoBase_PushMessageLogin(handle, 0, 0, 0)`。
9. 登录回调返回：
   - `0`：手机端确认成功，停止轮询并通知登录成功。
   - `-10516808`：手机端尚未确认，继续轮询，不显示错误。
   - `-10516809`：用户在手机端取消，停止轮询并允许重试。
   - 其他错误：停止轮询，显示错误并允许重试。

Qt 重写目标：

- 保留旧服务端接口调用顺序：`GetDynamicKey -> CancelPushMessageLogin -> SendPushMessage -> PushMessageLogin poll`。
- 继续使用 `push_message` 皮肤按钮，连接按钮位置和密码登录一致。
- 登录成功后走统一 `LoginWindowResult`，返回给 SDK 调用方。
- UI 层只做显示、按钮倒计时和轮询调度；SdoBase 细节集中在 `SdoBasePushMessageAuthenticator`。
