# External Link Configuration

Date: 2026-05-18

## Daoyu Behavior

The Daoyu-related entry buttons open the configured Daoyu page with the system default browser:

- QR login page: `需安装叨鱼`
- QR login page: `了解二维码登录详情`
- One-click login page: `需安装叨鱼`

## Configuration

The URL is read from `config/config.xml`:

```xml
<DaoyuHomeUrl value="https://www.daoyu8.com/#/" />
```

If the config value is missing, the runtime falls back to `https://www.daoyu8.com/#/`.

## Forgot Password Behavior

The main login page `忘记密码` entry opens the configured password recovery page with the system default browser.

Configuration:

```xml
<ForgotPasswordUrl value="https://i.sdo.com/index/findPassword" />
```

If the config value is missing, the runtime falls back to `https://i.sdo.com/index/findPassword`.

## Implementation Notes

- `sdologin/main.cpp` reads `DaoyuHomeUrl` into `LoginWindowContext`.
- `sdologin/main.cpp` reads `ForgotPasswordUrl` into `LoginWindowContext`.
- `LoginWindow` uses shared external-link helpers, so future external entries can share the same config-driven behavior.
