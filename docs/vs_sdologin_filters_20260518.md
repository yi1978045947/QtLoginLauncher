# sdologin Visual Studio Filters

Date: 2026-05-18

## Goal

Keep the `sdologin` project readable in Visual Studio by grouping files by login/runtime responsibility instead of leaving every file directly under `Source Files` and `Header Files`.

The grouping is defined in CMake through `source_group`, so regenerating the VS2019 solution keeps the same layout.

## Filters

- `入口与窗口`
  - `main`
  - command-line parsing
  - main Qt login window

- `扫码登录`
  - QR/code-key login model
  - sdobase QR auth adapter
  - QR login panel

- `静密登录`
  - legacy password auth adapter
  - password crypto bridge
  - sdobase password auth adapter

- `一键登录`
  - push-message/one-click skin handling

- `协议与Web`
  - protocol acceptance state
  - CEF/Qt protocol browser launcher
  - protocol browser style

- `虚拟键盘`
  - keyboard model
  - keyboard dialog

- `DPI与嵌入`
  - fixed-ratio UI scaling
  - Qt DPI setup
  - DX embedded-window style and alpha handling

## Notes

Generated Qt moc files are assigned to `Generated Files` through target properties where supported by CMake/Visual Studio.

Only the Visual Studio project organization changed here. Runtime behavior and exported SDK ABI are unchanged.
