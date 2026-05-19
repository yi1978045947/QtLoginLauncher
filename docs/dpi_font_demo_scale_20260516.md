# DPI 字体和 Demo 尺寸补充

日期：2026-05-16

## 问题

150% 缩放时，登录窗口和控件位置已经放大，但输入框文字、提示文字、按钮文字仍然偏小。原因是上一版只缩放了 QWidget 的 geometry，没有同步缩放：

- 继承字体；
- stylesheet 里的 `font:12px`、`font:16px`；
- stylesheet 里的 `padding-left:8px`、`border:1px` 等像素值。

## 当前修正

新增 `scaleLegacyStyleSheetPixels`，会把 stylesheet 中的 `Npx` 按 DPI scale 同步缩放。例如：

```text
font:12px -> font:18px
padding-left:8px -> padding-left:12px
border:1px -> border:2px
```

`LoginWindow` 当前会同时处理：

- 主窗口尺寸；
- 子控件 geometry；
- 根 QWidget 继承字体，默认按 `12px * scale`；
- 所有子控件 stylesheet 里的 `Npx`；
- 软键盘弹出偏移；
- 嵌入 DX 时的窗口尺寸。

## DX Demo 尺寸

DX demo 默认窗口从：

```text
960x540
```

改为：

```text
1280x720
```

原因是 150% 下登录器基准窗口 `442x404` 会变成 `663x606`，旧 demo 高度 540 会太挤。
