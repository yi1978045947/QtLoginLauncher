cmake_minimum_required(VERSION 3.20)

project(QtLoginRewrite LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "00_cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601)
if(MSVC)
    add_compile_options(/utf-8 /wd4828)
endif()

option(QTLOGIN_ENABLE_QT_UI "Build sdologin with Qt Widgets UI and Qt demo app" OFF)
option(QTLOGIN_ENABLE_CEF_OSR "Build protocol pages with CEF 109 off-screen rendering when a matching CEF package is available" ON)
option(QTLOGIN_LINK_LEGACY_AUTH_LIBS "Use the in-tree Win32 SdoBaseClient communication DLL for real legacy login calls" ON)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(qtlogin_default_cef109_root
        "${CMAKE_CURRENT_SOURCE_DIR}/CefProject/cef_binary_109.1.2+g2f7620c+chromium-109.0.5414.61_windows64_beta")
else()
    set(qtlogin_default_cef109_root
        "${CMAKE_CURRENT_SOURCE_DIR}/CefProject/cef_binary_109.1.2+g2f7620c+chromium-109.0.5414.61_windows32_beta")
endif()
set(QTLOGIN_CEF109_ROOT
    "${qtlogin_default_cef109_root}"
    CACHE PATH "CEF 109 binary package root")
set(QTLOGIN_CEF_WRAPPER_ROOT
    ""
    CACHE PATH "Optional prebuilt libcef_dll_wrapper root. Defaults to <CEF_ROOT>/win32 or <CEF_ROOT>/win64.")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config} config_upper)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/lib/${config})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/bin/${config})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/bin/${config})
endforeach()

add_subdirectory(third_party/sdologinclient)
add_subdirectory(src/common)
add_subdirectory(src/protocol)
add_subdirectory(src/ipc)
add_subdirectory(src/config)
add_subdirectory(src/ui_model)
add_subdirectory(src/sdk)
add_subdirectory(src/sdologin)
if(QTLOGIN_ENABLE_QT_UI)
    add_subdirectory(src/qtlogin_browser)
    if(TARGET sdologin AND TARGET QtLoginBrowser)
        add_dependencies(sdologin QtLoginBrowser)
    endif()
endif()
add_subdirectory(src/update_entry)
add_subdirectory(src/update_helper)
add_subdirectory(src/game_update)
add_subdirectory(samples/sdk_caller)
add_subdirectory(samples/qt_login_demo)
add_subdirectory(samples/dx_login_demo)
add_subdirectory(tests/protocol_tests)
add_subdirectory(tests/sdologin_tests)
add_subdirectory(tests/sdk_module_tests)
add_subdirectory(tests/config_tests)
add_subdirectory(tests/ui_model_tests)

function(qtlogin_set_vs_project target_name folder_name project_label)
    if(TARGET ${target_name})
        set_target_properties(${target_name} PROPERTIES
            FOLDER "${folder_name}"
            PROJECT_LABEL "${project_label}"
        )
    endif()
endfunction()

qtlogin_set_vs_project(sdk "01_runtime" "sdk")
qtlogin_set_vs_project(sdologin "01_runtime" "sdologin")
qtlogin_set_vs_project(SdoLoginClient "01_runtime" "SdoLoginClient")
qtlogin_set_vs_project(GameUpdate "01_runtime" "GameUpdate")
qtlogin_set_vs_project(updatehelper "01_runtime" "updatehelper")
qtlogin_set_vs_project(update_entry "01_runtime" "update_entry")

qtlogin_set_vs_project(libLoginClient "02_sdobaseclient" "libLoginClient")
qtlogin_set_vs_project(libUtil "02_sdobaseclient" "libUtil")
qtlogin_set_vs_project(SafeStoreCryptoHelper "02_sdobaseclient" "SafeStoreCryptoHelper")

qtlogin_set_vs_project(QtLoginCommon "03_qtlogin_internal" "QtLoginCommon")
qtlogin_set_vs_project(QtLoginConfig "03_qtlogin_internal" "QtLoginConfig")
qtlogin_set_vs_project(QtLoginIPC "03_qtlogin_internal" "QtLoginIPC")
qtlogin_set_vs_project(QtLoginProtocol "03_qtlogin_internal" "QtLoginProtocol")
qtlogin_set_vs_project(QtLoginUIModel "03_qtlogin_internal" "QtLoginUIModel")
qtlogin_set_vs_project(QtLoginBrowser "03_qtlogin_internal" "QtLoginBrowser")

qtlogin_set_vs_project(DXLoginDemo "04_demos" "DXLoginDemo")
qtlogin_set_vs_project(QtLoginDemo "04_demos" "QtLoginDemo")
qtlogin_set_vs_project(SDKCallerDemo "04_demos" "SDKCallerDemo")

qtlogin_set_vs_project(config_tests "05_tests" "config_tests")
qtlogin_set_vs_project(protocol_tests "05_tests" "protocol_tests")
qtlogin_set_vs_project(sdologin_tests "05_tests" "sdologin_tests")
qtlogin_set_vs_project(sdk_module_tests "05_tests" "sdk_module_tests")
qtlogin_set_vs_project(ui_model_tests "05_tests" "ui_model_tests")
