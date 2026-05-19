# DPI 自动缩放补充

日期：2026-05-16

## 问题

远程桌面或系统显示比例是 150% 时，登录器看起来没有放大。

根因是上一版 `dpi_scaler` 默认只读取：

- `QTLOGIN_LEGACY_DPI_SCALE`
- `QTLOGIN_LEGACY_DPI_PERCENT`
- SDK IPC 传入的 `legacyDpiScale` / `legacyDpiPercent`

如果这些都没传，默认走 `1.0`，所以系统 150% 不会自动影响登录窗口。

## 当前修正

现在缩放优先级改成：

```text
SDK 显式传入 legacyDpiScale / legacyDpiPercent
> 环境变量 QTLOGIN_LEGACY_DPI_SCALE / QTLOGIN_LEGACY_DPI_PERCENT
> 当前系统 DPI
> 96 DPI
```

也就是说，远程桌面 150% 通常会读到 `144 DPI`，最终：

```text
scale = 144 / 96 = 1.5
```

登录窗口基准尺寸：

```text
442x404 -> 663x606
```

## 日志确认

运行后看 `qtlogin_rewrite.log`，应该能看到：

```text
legacy dpi scale=1.5 systemDpi=144 envScale=0 loginSize=663x606
```

如果仍然是：

```text
legacy dpi scale=1 systemDpi=96
```

说明当前进程拿到的系统 DPI 还是 96，可能是远程桌面 DPI 没传给这个会话，或者应用在旧兼容模式下被系统虚拟化。此时可以临时用：

```bat
set QTLOGIN_LEGACY_DPI_PERCENT=150
dx_login_demo.exe
```

强制验证 1.5 倍缩放链路。
