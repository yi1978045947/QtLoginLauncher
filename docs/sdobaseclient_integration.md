# SdoBaseClient Integration

`sdologinclient\trunk` has been copied into this Qt rewrite as an in-tree third-party source dependency. The original sibling project is not modified.

## Source Layout

- `third_party/sdologinclient/include`: copied common headers and curl/openssl headers.
- `third_party/sdologinclient/libUtil`: copied utility sources plus `SafeStore.lib`.
- `third_party/sdologinclient/libLoginClient`: copied SdoBaseClient/LoginClient communication sources.
- `third_party/sdologinclient/SdoLoginClient`: copied DLL entry and original `SdoLoginClient.def`.
- `third_party/sdologinclient/output/Release`: copied runtime third-party DLLs and import libraries used by the communication DLL.

## Build Targets

- `sdobase_util`: builds the copied `libUtil` sources.
- `sdobase_loginclient`: builds the copied `libLoginClient` sources.
- `SdoBaseClient`: builds `SdoBaseClient.dll` with the original `SdoLoginClient.def` export ordinals.
- `qtlogin_sdobase_crypto`: small local helper DLL that isolates `SafeStore.lib` password encryption from the Qt `/MD` executable.

## Runtime Files

The following files are generated or copied beside `sdologin.exe` in Win32 builds:

- `SdoBaseClient.dll`
- `qtlogin_sdobase_crypto.dll`
- `libcurl.dll`
- `libssl-1_1.dll`
- `libcrypto-1_1.dll`
- `cacert.pem`

## Authentication Linkage

`sdologin.exe` defines `QTLOGIN_ENABLE_LEGACY_AUTH_LIBS` and links to the generated `SdoBaseClient` target when all of these are true:

- `QTLOGIN_LINK_LEGACY_AUTH_LIBS=ON`
- the build is Win32
- `SdoBaseClient` and `qtlogin_sdobase_crypto` targets exist

The Qt process calls the same SdoBase exported APIs for dynamic key and static password login. The helper DLL is only used for legacy password/Sdoa encryption so that communication callback results and exported SdoBaseClient ABI stay unchanged.
