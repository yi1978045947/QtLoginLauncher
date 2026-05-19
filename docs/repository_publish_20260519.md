# Repository Publish Notes

Date: 2026-05-19

Target repository:

```text
https://github.com/yi1978045947/QtLoginLauncher
```

## What Is Tracked

The repository is prepared to track the rewrite source, public headers, CMake project files, DuiLib reference assets, default config, merged SdoBaseClient code, samples, tests, and docs:

```text
assets/
cmake/
docs/
include/
samples/
source/
src/
tests/
third_party/
CMakeLists.txt
README.md
RUN_QT_DEMO.md
```

## What Is Ignored

The following are intentionally excluded:

- `build/`, `build_qt5_32/`, `build_qt5_64/`, `build_qt64/`: generated Visual Studio/CMake build outputs.
- `.qtcreator/`, `.vs/`, `.vscode/`, `*.user`: local IDE state.
- `CefProject/cef_binary_*`: local CEF 109 binary packages.
- CEF `.tar.bz2` archives: exceed GitHub's normal file size limit.
- Runtime logs and generated user config.

CEF setup is documented in `CefProject/README.md`.

## Verified Before Publish

The current local Win32 Qt 5.15 build was verified with:

```powershell
cmake --build qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin_tests -- /m
qtlogin_rewrite\build_qt5_32\bin\Debug\sdologin_tests.exe

cmake --build qtlogin_rewrite\build_qt5_32 --config Debug --target config_tests -- /m
qtlogin_rewrite\build_qt5_32\bin\Debug\config_tests.exe

cmake --build qtlogin_rewrite\build_qt5_32 --config Debug --target sdologin -- /m
cmake --build qtlogin_rewrite\build_qt5_32 --config Debug --target DXLoginDemo -- /m
```

Observed test outputs:

```text
sdologin_tests passed
config_tests passed
```

## Push Command

If GitHub network access or authentication is unavailable on this machine, push from `qtlogin_rewrite/` with:

```powershell
git remote add origin https://github.com/yi1978045947/QtLoginLauncher.git
git branch -M main
git push -u origin main
```
