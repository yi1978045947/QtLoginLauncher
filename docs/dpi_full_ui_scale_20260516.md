# DPI 全 UI 细节适配

日期：2026-05-16

## 目标

Qt 侧继续关闭 Qt 自动 DPI，保住 DX 嵌入窗口稳定性；登录器 UI 使用旧 Duilib 皮肤的 96 DPI 坐标，通过 `dpi_scaler` 按系统 DPI 做可控放大。

150% 缩放时，旧坐标 `x/y/w/h`、样式里的 `Npx`、字体像素、勾选框 indicator、输入框 placeholder、动态提示、键盘窗口和协议窗口都应同步变大。

## 已覆盖范围

- 主登录窗口：构造完成后递归缩放子控件 geometry、stylesheet 里的 `px` 和 QWidget 字体。
- 输入框：`editStyle()` 显式设置 `font:12px` 和 placeholder 颜色，随后由 stylesheet 缩放器放大。
- 勾选框：indicator 使用 `border-image`，宽高和图片一起随 DPI 缩放。
- 动态提示：`showHint()` 每次显示时按当前 `legacyDpiScaleFactor_` 缩放位置、圆角、padding 和字体。
- 安全键盘：新增 `legacyKeyboardWindowRect()`、`legacyKeyboardKeyRectForPosition()`、`legacyKeyboardControlRect()`，键盘窗口、随机键位、Back/Shift/Enter/X、说明文字和按钮字体全部按同一比例缩放。
- 旧窗口清单/骨架弹框：运行时创建时缩放 fixed size、layout margin/spacing 和 stylesheet。
- 协议浏览器进程：`QtLoginBrowser` 也链接 `dpi_scaler.cpp`，独立进程内按系统 DPI 缩放窗口、边距、按钮高度和 stylesheet。

## 验证

`sdologin_tests` 增加了键盘缩放模型断言：

- `legacyKeyboardWindowRect(1.5)` 输出 `734 x 312`。
- 第一颗键从 `18,18,26,22` 放大为 `27,27,39,33`。
- 键盘提示文字区域从 `10,164,260,22` 放大为 `15,246,390,33`。

构建命令：

```bat
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin_tests
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target DXLoginDemo
```
