add_library(sdk SHARED
    sdk_exports.cpp
    legacy_login_adapter.cpp
    legacy_login_adapter.h
    sdk_runtime.cpp
    sdk_runtime.h
    sdoa_app_adapter.cpp
    sdoa_app_adapter.h
    login_process_manager.cpp
    login_process_manager.h
    directx_compat_adapter.cpp
    directx_compat_adapter.h
    sdk.def
)

source_group("Source Files\\SDK Exports" FILES
    sdk_exports.cpp
)
source_group("Source Files\\SDK Runtime" FILES
    sdk_runtime.cpp
)
source_group("Header Files\\SDK Runtime" FILES
    sdk_runtime.h
)
source_group("Source Files\\SDOA4Client Compat" FILES
    sdoa_app_adapter.cpp
)
source_group("Header Files\\SDOA4Client Compat" FILES
    sdoa_app_adapter.h
)
source_group("Source Files\\ISDOLLogin Compat" FILES
    legacy_login_adapter.cpp
)
source_group("Header Files\\ISDOLLogin Compat" FILES
    legacy_login_adapter.h
)
source_group("Source Files\\Login Process" FILES
    login_process_manager.cpp
)
source_group("Header Files\\Login Process" FILES
    login_process_manager.h
)
source_group("Source Files\\DirectX Compat" FILES
    directx_compat_adapter.cpp
)
source_group("Header Files\\DirectX Compat" FILES
    directx_compat_adapter.h
)
source_group("Export Files" FILES
    sdk.def
)

target_include_directories(sdk PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/ipc
    ${PROJECT_SOURCE_DIR}/src/protocol
)

target_link_libraries(sdk PRIVATE
    QtLoginCommon
    QtLoginIPC
    QtLoginProtocol
    ole32
    oleaut32
)

set_target_properties(sdk PROPERTIES
    OUTPUT_NAME "sdologinsdk"
)
