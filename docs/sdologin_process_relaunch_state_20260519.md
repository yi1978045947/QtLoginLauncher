# sdologin Process Relaunch And Legacy Node State

Date: 2026-05-19

## Legacy Reference

Old DuiLib `sdologin.exe` uses `trunk/src/Login/SDOLApp.h/.cpp` to join a local process-node group:

- `StartupNodes()` starts `CLoginModule2`, `CPluginModule`, and `IGAOutGameModule` with one generated group id.
- `m_LoginModule.SendLoginModuleLaunchedNotification()` tells the SDK-side node that login module is alive.
- `OnConfirmLoginModuleLaunched()` records the SDK node id/module id and starts `TimerForCheckSdkExist`.
- `TimerForCheckSdkExist` exits `sdologin.exe` when the SDK node disappears, except launcher login mode can keep waiting after login success.
- `TimerForLaunchedResponseTimeout` exits if the SDK never confirms the launched notification.

The old code comments call this storage "网络", but it is not the game server or HTTP network. It is local proc-node shared state exposed through `m_LoginModule.SetUserData / GetUserData / RemoveUserData`. The saved keys are scoped by the process-node id, for example:

- `SDOL_LOGIN_GAMEWNDHANDLE`
- `SDOL_LOGIN_APPINFO`
- `SDOL_LOGIN_LOGINRESULT`
- `SDOL_LOGIN_LOGIN_DIALOG_VISIBLE`
- `SDOL_LOGIN_TGT`
- `SDOL_LOGIN_MODE`
- `SDOL_WINDOW_MODE`
- `SDOL_WINDOW_POS`
- `SDOL_LOGIN_LAUNCHERFLAG`
- `SDOL_V3_LAUNCHER_GUID`

`RecoverOldStatus()` reads those keys after reattach and restores owner window, app info, login state/session, login mode, window mode, visibility, and position.

## Qt Rewrite Approach

The Qt rewrite does not recreate the whole old proc-node framework. The current SDK owns the authoritative runtime state in `src/sdk/sdk_runtime.cpp` and sends it to each launched `sdologin.exe` through the named-pipe `ShowLogin` command:

- app id/name/version/area/group
- login mode
- owner window handle
- DirectX embedded flag
- legacy DPI scale
- login window position
- game client type
- server id
- reserved value

This is the Qt equivalent of restoring the old node snapshot for the login dialog path: if the first `sdologin.exe` dies before returning a login result, `SdkRuntime::showLoginDialog()` launches a fresh `sdologin.exe`, creates a new pipe, and resends the same snapshot.

## Implemented Behavior

`SdkRuntime::showLoginDialog()` now retries the login process once by default:

1. Launch `sdologin.exe`.
2. Complete the pipe `Hello / HelloAck`.
3. Send `ShowLogin` with the current SDK state snapshot.
4. Wait for `LoginSuccess`, `LoginCancel`, or `LoginError`.
5. If the pipe closes before a result, log the process exit code, terminate/close the stale process handle, relaunch `sdologin.exe`, and resend the same `ShowLogin` state.

The retry can be disabled for diagnostics with:

```bat
set QTLOGIN_DISABLE_LOGIN_PROCESS_RELAUNCH=1
```

Test-only flags:

```bat
set QTLOGIN_MOCK_SUCCESS=1
set QTLOGIN_TEST_EXIT_FIRST_LOGIN_PROCESS=1
```

Those make the first `sdologin.exe` exit after receiving `ShowLogin`, then make the relaunched process return a mock success result.

## Compatibility Notes

- This does not change exported SDK ABI.
- Old callers still use `ShowLoginDialog` / `ISDOAApp::ShowLoginDialog` as before.
- The Qt path keeps state inside the SDK process instead of a shared proc-node data table. That is simpler and safer for the rewritten IPC design because the SDK is already the only component that knows the caller's ABI state.
- A full old-style node table would only be needed later if independent helper processes must query/update login state while no synchronous SDK call is active.

## Verification

```bat
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdk sdologin sdk_module_tests sdologin_tests -- /m
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdk_module_tests.exe
```

`sdk_module_tests` includes `verifySdolLoginProcessRelaunch`, which verifies that `ShowLoginDialog` succeeds after the first login process exits before returning a result.
