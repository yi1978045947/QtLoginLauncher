# 一键登录迁移记录 2026-05-18

## 旧工程入口

参考文件：

- `trunk/src/Login/PushMessageLoginDlg.h`
- `trunk/src/Login/PushMessageLoginDlg.cpp`
- `trunk/src/Login/Authen.h`
- `trunk/src/Login/Authen.cpp`

旧版 `CPushMessageLoginDlg` 的按钮流程是：

1. `OnBtnLogin`
   - 检查协议是否勾选。
   - 更新 AppInfo。
   - 调用 `AsyncGetDynamicKey`。
2. `OnGetDynamicKeyResult`
   - 动态密钥失败则提示。
   - 校验账号，移动账号限制开启时只允许数字和 `+`。
   - 调用 `AsyncCacelPushMessageLogin` 清理上一轮一键登录状态。
3. `OnCancelSendPushMessageResult`
   - 调用 `AsyncSendPushMessage`。
4. `AsyncSendPushMessage`
   - 调用 `SdoBase_SendPushMessage(handle, account, scene)`。
   - Windows XP 使用 `xp_pushmsglogin`，其他系统使用 `pc_pushmsglogin`。
5. `OnSendPushMessageResult`
   - 成功后显示确认码，启动 1 秒轮询。
   - 按钮切换成 `push_message/6_*.png` 重试态，倒计时 6 秒。
6. 轮询
   - 调用 `SdoBase_PushMessageLogin(handle, 0, 0, 0)`。
   - `0` 登录成功。
   - `-10516808` 用户未确认，继续轮询。
   - `-10516809` 手机端取消，停止轮询并允许重试。

## Qt 重写实现

新增文件：

- `src/sdologin/push_message_login_model.h/.cpp`
  - 一键登录账号校验。
  - `pc_pushmsglogin/xp_pushmsglogin` scene 选择。
  - `-10516808/-10516809/0` 登录结果转换。
- `src/sdologin/sdo_base_push_message_auth.h/.cpp`
  - 真实调用 `SdoBaseClient`。
  - 调用顺序保持为 `GetDynamicKey -> CancelPushMessageLogin -> SendPushMessage -> PushMessageLogin`。
  - 回调统一转换成 Qt 自己的 `PushMessageSendResult` 和 `PushMessageLoginResult`。
  - 旧工程通过窗口消息等待 `CancelPushMessageLogin` 回调后再发送 push。当前 `SdoBaseClient` 在“没有上一轮 push”时可能只返回 0 但不触发 cancel 回调，所以 Qt 版在 cancel 接口返回 0 后立即继续 `SendPushMessage`，并用幂等标记避免后续 cancel 回调又到达时重复发送。
- `src/sdologin/push_message_login_panel.h/.cpp`
  - 一键登录 Qt 面板。
  - 使用 `push_message` 资源和共用连接按钮位置。
  - 负责按钮倒计时、状态文字、1 秒轮询、叨鱼官网外部打开。
- `src/sdologin/login_window.cpp`
  - `buildQuickLoginPage` 改为创建 `PushMessageLoginPanel`。
  - 一键登录成功后调用 `completePushMessageLogin`，再通过原 IPC 返回 SDK。

## 当前简化点

- 一键登录已经接入真实服务端接口链路。
- 暂未复刻旧版三个状态图标的细粒度显示。
- 叨鱼安装状态、版本状态、在线状态、开关状态、黑名单状态后续可继续接 `SdoBase_GetPushMessageStatus`。
- 图验、实名、监护人等二次流程仍沿用当前阶段策略：先返回提示，后续再按弹窗逐个补齐。

## 验证

- `cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin_tests sdologin`
- `.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe`
