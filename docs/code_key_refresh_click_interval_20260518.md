# 扫码刷新二维码点击间隔 2026-05-18

## 需求

扫码登录界面的“刷新二维码”按钮增加配置项：

```xml
<!--ClickInterval 扫码界面刷新二维码间隔时间-->
<ClickInterval value="1500" />
```

连续点击时，只有距离上一次有效点击超过配置毫秒数后，才继续调用服务端刷新二维码接口。默认值为 `1500` 毫秒；配置缺失、为 0 或负数时也按 `1500` 毫秒处理。

## 实现位置

- `assets/default_config/config/config.xml`
  - 新增默认 `ClickInterval` 配置。
- `src/sdologin/main.cpp`
  - 启动读取配置，写入 `LoginRuntimeContext::codeKeyRefreshClickIntervalMs`。
- `src/sdologin/login_window.h/.cpp`
  - 将运行时配置传给扫码登录面板。
- `src/sdologin/code_key_login_model.h/.cpp`
  - 新增 `normalizedCodeKeyRefreshClickIntervalMs` 和 `shouldAllowCodeKeyRefreshClick`，方便单元测试覆盖间隔判断。
- `src/sdologin/code_key_login_panel.h/.cpp`
  - 刷新按钮点击先走本地节流判断；未达到间隔时直接忽略，不再调用 `startNewFlow()`，也就不会触发 `SdoBase_GetDynamicKey -> SdoBase_GetQrCode`。

## 说明

本次只限制用户手动点击“刷新二维码”按钮。页面首次进入、协议通过后自动拉码、切换页面重新进入等流程仍保持原逻辑，不受 `ClickInterval` 限制。

## 验证

- `cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin_tests sdologin DXLoginDemo`
- `.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe`
