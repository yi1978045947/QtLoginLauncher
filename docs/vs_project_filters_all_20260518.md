# Visual Studio Project Filters

Date: 2026-05-18

## Goal

The VS2019 solution now uses project-level folders and per-project filters.

Project folders show the big module boundary:

- `01_runtime`
- `02_sdobaseclient`
- `03_qtlogin_internal`
- `04_demos`
- `05_tests`

Inside each project, source files are grouped by responsibility with CMake `source_group`, so the layout is regenerated consistently.

## Runtime Projects

- `sdk`
  - `SDK导出`
  - `SDK运行时`
  - `旧接口适配`
  - `登录进程管理`
  - `DX嵌入兼容`
  - `Export Files`

- `sdologin`
  - `入口与窗口`
  - `扫码登录`
  - `静密登录`
  - `一键登录`
  - `协议与Web`
  - `虚拟键盘`
  - `DPI与嵌入`
  - `Generated Files`

- `GameUpdate`
  - `更新SDK导出`
  - `Export Files`

- `update_entry`
  - `更新入口`
  - `Export Files`

- `updatehelper`
  - `更新辅助进程`

## SdoBaseClient Projects

- `libUtil`
  - `JSON`
  - `MD5`

- `libLoginClient`
  - `基础工具`
  - `HTTP通信`
  - `加密与握手`
  - `登录客户端`

- `SdoLoginClient`
  - `DLL入口`

- `SafeStoreCryptoHelper`
  - `加密导出`

## QtLogin Internal Projects

- `QtLoginCommon`
  - `日志`
  - `进程工具`
  - `字符串工具`

- `QtLoginConfig`
  - `配置模型`
  - `配置读取`

- `QtLoginIPC`
  - `命名管道`

- `QtLoginProtocol`
  - `JSON载荷`
  - `协议帧`
  - `协议类型`

- `QtLoginUIModel`
  - `旧窗口映射`
  - `UI模型`

- `QtLoginBrowser`
  - `浏览器入口`
  - `Web通信桥`
  - `CEF离屏渲染`
  - `协议窗口`
  - `DPI适配`
  - `Generated Files`

## Demo And Test Projects

- `DXLoginDemo`
  - `Demo入口`
  - `DX渲染与SDK调用`

- `QtLoginDemo`
  - `Demo入口`

- `SDKCallerDemo`
  - `SDK调用示例`

- test projects
  - `测试入口`
  - `sdologin_tests` additionally groups copied production sources under `被测代码`.

## Notes

This change only affects Visual Studio organization. It does not change SDK exports, process communication, login behavior, or generated binaries.
