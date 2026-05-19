# 密码加密桥接 DLL 导出名修正

日期：2026-05-16

## 现象

密码登录时界面提示：

```text
Failed to load QtLogin_MakeLegacyPasswordTextBase64 from qtlogin_sdobase_crypto.dll.
```

这个提示不是服务端返回，也不是账号密码错误。它表示 `sdologin.exe` 已经进入密码登录流程，但在本机加载旧 SafeStore 密码加密桥接 DLL 时，`GetProcAddress` 没找到导出函数。

## 原因

Win32 下 `__stdcall` 导出函数默认会带符号修饰，例如：

```text
_QtLogin_MakeLegacyPasswordTextBase64@16
_QtLogin_SdoaEncryptWithDynamicKey@20
```

而调用侧按未修饰名查找：

```text
QtLogin_MakeLegacyPasswordTextBase64
QtLogin_SdoaEncryptWithDynamicKey
```

所以还没真正执行 RSA 密码文本生成，就提前失败了。

## 修正

- 新增 `qtlogin_sdobase_crypto.def`，固定导出未修饰函数名。
- `SafeStoreCryptoHelper` 在 Win32 MSVC 构建时使用该 `.def`。
- 调用侧保留兼容兜底：先找未修饰名，找不到再找 Win32 `__stdcall` 修饰名。
- `sdk_module_tests` 增加回归检查，直接验证 `qtlogin_sdobase_crypto.dll` 的两个未修饰导出是否存在。

## 验证

```bat
cmake --build .\qtlogin_rewrite\build_qt5_32 --config Debug --target sdk_module_tests
.\qtlogin_rewrite\build_qt5_32\bin\Debug\sdk_module_tests.exe
```
