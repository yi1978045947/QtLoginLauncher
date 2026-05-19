# CEF 109 Local Package

CEF 109 binaries are intentionally not committed to this Git repository.

Reasons:

- The extracted CEF packages are several gigabytes.
- The `.tar.bz2` archives are larger than GitHub's normal 100 MB file limit.
- Build outputs copy `libcef.dll`, resources, and subprocess files into local `build_*` directories.

Place the matching CEF package under this directory when CEF off-screen protocol pages are needed.

Expected package directories:

```text
CefProject/
  cef_binary_109.1.2+g2f7620c+chromium-109.0.5414.61_windows32_beta/
  cef_binary_109.1.2+g2f7620c+chromium-109.0.5414.61_windows64_beta/
```

The current Win7-compatible path is Qt 5.15.2 + VS2019 Win32, so the Win32 CEF package is the important one:

```powershell
cmake -S . -B build_qt5_32 -G "Visual Studio 16 2019" -A Win32 `
  -DQTLOGIN_ENABLE_QT_UI=ON `
  -DQTLOGIN_ENABLE_CEF_OSR=ON `
  -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\msvc2019
```

If CEF is unavailable, `QtLoginBrowser` still builds a fallback protocol browser path, but real agreement pages should be tested with CEF present.
