# DPI 勾选框和提示文字细节

日期：2026-05-16

## 问题

150% 缩放下，登录窗口主体已经变大，但细节还有两类不协调：

- 勾选框看起来仍是原始小图，未按 indicator 区域拉伸；
- 动态提示文字 `hintLabel` 是点击后临时创建的，创建时没有走启动后的 DPI 缩放流程。

## 修正

勾选框样式从 `image:` 改为 `border-image:`：

```text
QCheckBox::indicator {
    width:18px;
    height:15px;
    border:0;
    border-image:url(...);
}
```

后续 `scaleLegacyStyleSheetPixels` 会把 `18px/15px` 按比例缩放，`border-image` 会跟随 indicator 大小拉伸皮肤图。

提示文字现在在 `showHint` 创建/显示时实时使用当前 `legacyDpiScaleFactor_`：

- geometry 从 `95,118,300,26` 按 scale 缩放；
- `font:13px`、`border-radius:3px`、padding 同步缩放；
- 这样后续动态创建的提示控件不会漏掉 DPI 处理。
