# SdoLoginClient Merge Plan

Goal: bring the VS2019 SdoBaseClient communication project into `qtlogin_rewrite` without changing the original `sdologinclient\trunk` directory and without changing exported communication results.

## Scope

- Copy the source and required prebuilt third-party binaries from `..\sdologinclient\trunk` into `qtlogin_rewrite\third_party\sdologinclient`.
- Build the copied code from the Qt CMake solution as:
  - `sdobase_util`: static library from `libUtil`.
  - `sdobase_loginclient`: static library from `libLoginClient`.
  - `SdoBaseClient`: DLL using the original `SdoLoginClient.def` export names and ordinals.
- Keep `sdologin.exe` as a DLL client of `SdoBaseClient.dll`, rather than statically merging server communication code into the Qt process.
- Preserve callback signatures, exported symbols, request/response parsing, and error/result payload behavior from the copied SdoBaseClient code.

## Integration Rules

- Do not edit files under the original sibling directory `..\sdologinclient\trunk`.
- Local Qt-side changes are allowed under `qtlogin_rewrite`.
- Runtime DLLs copied beside `sdologin.exe`: `SdoBaseClient.dll`, `libcurl.dll`, `libssl-1_1.dll`, `libcrypto-1_1.dll`, and `cacert.pem`.
- Real SdoBase authentication is enabled only for Win32 builds because the imported project only has Win32 configurations and binary dependencies.

## Verification

- Configure and build `build_qt5_32` Debug.
- Verify `SdoBaseClient.dll` is generated in `build_qt5_32\bin\Debug`.
- Verify `sdologin.exe` links with `QTLOGIN_ENABLE_LEGACY_AUTH_LIBS`.
- Run existing module tests after the build.
