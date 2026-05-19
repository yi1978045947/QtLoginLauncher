# 一键登录取消回调与 DPI 按钮修复记录 2026-05-18

## 问题现象

- 点击一键登录后，连接按钮变小、位置不对。
- 日志中 `SdoBase_SendPushMessage` 返回 `-10130002`。

## 根因

1. 一键登录按钮在初始化后会被 `dpi_scaler` 按旧皮肤坐标整体放大。发送失败或进入重试态时，`PushMessageLoginPanel` 又重新调用按钮皮肤函数并写回 1x 旧坐标，覆盖了 DPI 缩放后的 geometry。
2. 老工程不是直接使用 `SdoBase_SetCancelPushMessageCallBack` 注册取消回调，而是在 `Authen.cpp` 中通过：

   ```cpp
   SdoBase_SetCallBack(m_pSdoBaseHandle, SdoBase_CancelPushMessageCallback, onCancelPushMessageCallback);
   ```

   当前合入的 VS2019 `SdoBaseClient` 里，`ProcessCancelPushMessageResponse` 只从内部 callback map 读取 `SdoBase_CancelPushMessageCallback`。因此 Qt 版之前的取消回调没有被真正触发。

3. 因为取消回调没有回来，之前为了避免卡住曾在 `SdoBase_CancelPushMessageLogin` 返回 0 后直接调用 `SdoBase_SendPushMessage`。这时取消请求还在处理，`SdoBaseClient` 的 `ProcessRequest` 处于 busy 状态，所以返回 `ERROR_PROCESSING(-10130002)`。
4. 老工程的 `onCancelPushMessageCallback` 不判断 `resultCode`，无论服务端是否返回 `-10242301` 这类“输入有误”，都会发送 `WM_LOGIN_CANCEL_SEND_PUSHMESSAGE` 并继续进入 `AsyncSendPushMessage`。因此取消旧 push 会话失败不能阻断当前一键登录发送。

## 修复

- `sdo_base_push_message_auth.cpp`
  - 改为使用 `SdoBase_SetCallBack(handle_, SdoBase_CancelPushMessageCallback, onCancelPushMessageCallback)` 注册取消回调。
  - `CancelPushMessageLogin` 返回 0 后等待 `onCancelPushMessageCallback`，在回调中再调用 `SdoBase_SendPushMessage`，与老工程链路一致。
  - `CancelPushMessageCallback` 的业务错误不再直接展示给用户，按老工程行为继续进入 `SdoBase_SendPushMessage`。

- `push_message_login_panel.cpp`
  - 初始化按钮时写入旧皮肤 geometry。
  - 运行时切换“连接/重试”按钮图片时只改 stylesheet，不再写 geometry，避免覆盖 DPI 缩放结果。

## 当前一键登录接口链路

1. `SdoBase_GetDynamicKey`
2. `SdoBase_CancelPushMessageLogin`
3. `SdoBase_CancelPushMessageCallback`
4. `SdoBase_SendPushMessage`
5. `SdoBase_SendPushMessageCallback`
6. `SdoBase_PushMessageLogin` 轮询

其中 `SdoBase_GetPushMessageStatus` 是老工程已接回调的状态查询接口，当前点击登录主链路没有直接调用；后续可以按状态提示需求单独接到一键登录面板。

## 验证

- `cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin_tests sdologin DXLoginDemo`
- `.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe`

补充验证：当 `QtLoginBrowser`/CEF 文件被运行中的 demo 或登录器占用时，完整依赖构建会在复制 CEF Debug 目录时报错。可先关闭相关进程再跑完整构建；本次改动已用以下命令验证受影响目标：

- `.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe`
- `cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin -- /p:BuildProjectReferences=false`
- `cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target DXLoginDemo -- /p:BuildProjectReferences=false`
