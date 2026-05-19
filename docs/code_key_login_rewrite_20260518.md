# 扫码登录迁移记录 2026-05-18

## 老工程流程

参考文件：

- `trunk/src/Login/CodeKeyLoginDlg.h`
- `trunk/src/Login/CodeKeyLoginDlg.cpp`
- `trunk/src/Login/Authen.h`
- `trunk/src/Login/Authen.cpp`

老版 `CCodeKeyLoginDlg` 的主流程：

1. 进入扫码页 `Show()` 后，如果协议已勾选，设置登录窗口句柄并调用 `CAuthenManager::AsyncGetDynamicKey(this)`。
2. `Authen.cpp::onGetDynamicKeyCallback` 收到回调后投递 `WM_LOGIN_GET_DYNAMICKEY`。
3. `CCodeKeyLoginDlg::OnGetDynamicKeyResult` 成功后设置 `maxsize`，调用 `AsyncGetQrCode(this)`。
4. `Authen.cpp::AsyncGetQrCode` 调用 `SdoBase_GetQrCode`。
5. `Authen.cpp::onGetQrCodeCallback` 返回二维码图片二进制，投递 `WM_LOGIN_GETCODEKEY_RESULT`。
6. `CCodeKeyLoginDlg::OnShowCodeKey` 用 GDI+ 从内存流解析二维码图片并贴到 `CodeKeyLabel`，随后开启 1 秒定时器。
7. 定时器触发后调用 `AsyncQrCodeLogin(this)`，内部调用 `SdoBase_QrCodeLogin(handle, 0, 0, 0)`。
8. `Authen.cpp::onLoginResultCallback` 返回登录结果：
   - `0`：登录成功，停止轮询并发出登录成功通知。
   - `-10515805`：继续轮询；其中 `isScanned == "1"` 表示手机端已扫码但未确认。
   - `-10515801`：二维码失效，停止轮询并显示刷新态。
   - 其他错误：停止轮询并显示错误提示。

## Qt 重写实现

本次新增独立扫码登录类，避免继续把二维码逻辑塞到 `LoginWindow`：

- `src/sdologin/code_key_login_model.h/.cpp`
  - 纯状态模型，负责把老 `SdoBase` 登录回调转换成 Qt 扫码状态。
  - 定义 `CodeKeyLoginState`、`CodeKeyLoginResult`、`kCodeKeyContinuePollingErrorCode`、`kCodeKeyExpiredErrorCode`。

- `src/sdologin/sdo_base_code_key_auth.h/.cpp`
  - 新的 `SdoBaseCodeKeyAuthenticator`。
  - 初始化参数复用 `SdoBaseAuthConfig`。
  - 按老流程调用 `SdoBase_GetDynamicKey -> SdoBase_GetQrCode -> SdoBase_QrCodeLogin`。
  - 拉二维码前设置 `maxsize`，刷新时调用 `SdoBase_ClearCodeKey`。
  - 轮询中遇到 `ERROR_PROCESSING` 时按继续轮询处理，避免 UI 误报失败。

- `src/sdologin/code_key_login_panel.h/.cpp`
  - 新的 Qt 扫码登录面板类。
  - 使用 `skin/code_key/QR_bg.png`、`btn_er_*`、`QR_btn_*`、`bg_success1.png`、`bg_failed.png` 等旧皮肤资源。
  - 页面激活时如果协议已勾选就自动开始拉码；切走页面时停止轮询并取消当前请求。
  - “需安装叨鱼”和“了解二维码登录详情”走配置的 `DaoyuHomeUrl`。

- `src/sdologin/login_window.cpp`
  - `buildQrPage()` 改为创建 `CodeKeyLoginPanel`。
  - 移除原来的“点击模拟扫码成功”逻辑。
  - 扫码成功后通过真实 `CodeKeyLoginResult` 回填 `sessionId/sndaid/ticket` 并走 SDK 完成回调。

## 验证

已验证：

- `sdologin_tests.exe`
- `sdologin` Debug Win32 构建
- `DXLoginDemo` Debug Win32 构建
- `SDKCallerDemo` Debug Win32 构建

说明：真实服务端扫码结果需要用可用账号和叨鱼/微信扫码端联调确认，本次代码已接到 `SdoBaseClient.dll` 真实接口链路。
