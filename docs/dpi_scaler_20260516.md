# Qt 登录器 DPI 策略

日期：2026-05-16

## 结论

本工程不能直接依赖 Qt 自动高 DPI 缩放。原因是 SDK/DX 调用方传入的窗口位置、大小和鼠标命中区域属于 Win32/DX 物理像素体系，而 Qt 开启 `AA_EnableHighDpiScaling` 后 QWidget 使用逻辑像素。两套坐标混用会导致嵌入窗口尺寸、mask、点击区域和旧皮肤坐标错位。

所以当前策略是：

1. 先恢复旧 SDK 友好的 legacy DPI 模式，保证 `Qt 坐标 == Win32/DX 物理像素坐标`。
2. 不打开 Qt 自动高 DPI。
3. 通过 `dpi_scaler` 按旧皮肤坐标做可控缩放。

## legacy DPI 模式

`src/sdologin/qt_dpi_config.cpp` 当前固定：

```text
QT_AUTO_SCREEN_SCALE_FACTOR=0
QT_ENABLE_HIGHDPI_SCALING=0
QT_SCALE_FACTOR=1
QT_FONT_DPI=96
AA_Use96Dpi
AA_DisableHighDpiScaling
```

进程 DPI awareness 保持 system aware。这样 DX 父窗口、SDK 传参、`SetWindowPos` 和 Qt widget 坐标不会被 Qt 自动换算。

## dpi_scaler 规则

新增文件：

- `src/sdologin/dpi_scaler.h`
- `src/sdologin/dpi_scaler.cpp`

核心规则：

```text
scale = dpi / 96
scale = percent / 100
```

例子：

```text
96 DPI / 100%  -> 1.00
120 DPI / 125% -> 1.25
144 DPI / 150% -> 1.50
```

像素坐标使用四舍五入：

```text
442 * 1.25 = 553
QRect(49, 204, 108, 44) * 1.25 = QRect(61, 255, 135, 55)
```

## 当前接入范围

`LoginWindow` 当前已经支持基础可控缩放：

- 主登录窗口基准尺寸 `442x404` 会按 `dpi_scaler` 缩放。
- 登录窗口子控件的旧皮肤坐标会按同一个比例递归缩放。
- 嵌入 DX 时 `SetWindowPos` 使用缩放后的窗口尺寸。
- 背景 mask 会按缩放后的窗口尺寸重新生成。

默认不自动读取系统 DPI，默认 scale 为 `1.0`，先保证 DX 嵌入稳定。

## 调试/试验开关

可以用环境变量手动试验：

```bat
set QTLOGIN_LEGACY_DPI_PERCENT=125
dx_login_demo.exe
```

或：

```bat
set QTLOGIN_LEGACY_DPI_SCALE=1.25
dx_login_demo.exe
```

运行日志会记录：

```text
legacy dpi scale=1.25 loginSize=553x505
```

后续如果要自动适配系统 DPI，应在 SDK/调用方边界明确“传入的是物理像素还是逻辑像素”，再决定是否从父窗口 DPI 推导 scale。
