find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
include(${PROJECT_SOURCE_DIR}/cmake/QtDeploy.cmake)

set(qtlogin_browser_sources
    main.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/cef_web_bridge.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/cef_web_bridge.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/dpi_scaler.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/dpi_scaler.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_acceptance_model.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_acceptance_model.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_dialog.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_style.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_style.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/qt_dpi_config.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/qt_dpi_config.h
)

set(qtlogin_cef_osr_available OFF)
set(qtlogin_cef_osr_unavailable_reason "CEF OSR is disabled by CMake.")
if(QTLOGIN_ENABLE_CEF_OSR)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(qtlogin_cef_wrapper_arch "win64")
    else()
        set(qtlogin_cef_wrapper_arch "win32")
    endif()
    set(qtlogin_cef_wrapper_md_root
        "${QTLOGIN_CEF109_ROOT}/${qtlogin_cef_wrapper_arch}_md/libcef_dll_wrapper")
    set(qtlogin_cef_wrapper_default_root
        "${QTLOGIN_CEF109_ROOT}/${qtlogin_cef_wrapper_arch}/libcef_dll_wrapper")

    if(QTLOGIN_CEF_WRAPPER_ROOT)
        set(qtlogin_cef_wrapper_root "${QTLOGIN_CEF_WRAPPER_ROOT}")
    elseif(EXISTS "${qtlogin_cef_wrapper_md_root}/Release/libcef_dll_wrapper.lib")
        set(qtlogin_cef_wrapper_root "${qtlogin_cef_wrapper_md_root}")
    else()
        set(qtlogin_cef_wrapper_root "${qtlogin_cef_wrapper_default_root}")
    endif()

    if(NOT WIN32)
        set(qtlogin_cef_osr_unavailable_reason "CEF OSR currently targets Win32 builds only.")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8 AND QTLOGIN_CEF109_ROOT MATCHES "windows32")
        set(qtlogin_cef_osr_unavailable_reason "CEF 109 package under CefProject is windows32, but this Qt build is 64-bit.")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4 AND QTLOGIN_CEF109_ROOT MATCHES "windows64")
        set(qtlogin_cef_osr_unavailable_reason "CEF 109 package under CefProject is windows64, but this Qt build is 32-bit.")
    elseif(NOT EXISTS "${QTLOGIN_CEF109_ROOT}/Release/libcef.lib")
        set(qtlogin_cef_osr_unavailable_reason "CEF 109 Release/libcef.lib was not found.")
    elseif(NOT EXISTS "${qtlogin_cef_wrapper_root}/Release/libcef_dll_wrapper.lib")
        set(qtlogin_cef_osr_unavailable_reason "CEF 109 libcef_dll_wrapper.lib was not found.")
    else()
        set(qtlogin_cef_osr_available ON)
        set(qtlogin_cef_osr_unavailable_reason "")
        list(APPEND qtlogin_browser_sources
            ${PROJECT_SOURCE_DIR}/src/sdologin/cef_offscreen_browser.cpp
            ${PROJECT_SOURCE_DIR}/src/sdologin/cef_offscreen_browser.h
        )
        source_group("Source Files\\CEF离屏渲染" FILES
            ${PROJECT_SOURCE_DIR}/src/sdologin/cef_offscreen_browser.cpp
        )
        source_group("Header Files\\CEF离屏渲染" FILES
            ${PROJECT_SOURCE_DIR}/src/sdologin/cef_offscreen_browser.h
        )
    endif()
endif()

add_executable(QtLoginBrowser WIN32 ${qtlogin_browser_sources})
set_target_properties(QtLoginBrowser PROPERTIES
    OUTPUT_NAME "qtlogin_browser"
    AUTOMOC ON
    AUTOGEN_SOURCE_GROUP "Generated Files"
    AUTOMOC_SOURCE_GROUP "Generated Files"
)

source_group("Source Files\\浏览器入口" FILES
    main.cpp
)
source_group("Source Files\\Web通信桥" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/cef_web_bridge.cpp
)
source_group("Header Files\\Web通信桥" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/cef_web_bridge.h
)
source_group("Source Files\\协议窗口" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_acceptance_model.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_dialog.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_style.cpp
)
source_group("Header Files\\协议窗口" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_acceptance_model.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_dialog.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_style.h
)
source_group("Source Files\\DPI适配" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/dpi_scaler.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/qt_dpi_config.cpp
)
source_group("Header Files\\DPI适配" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/dpi_scaler.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/qt_dpi_config.h
)
source_group("Resource Files\\清单" FILES
    qtlogin_browser.exe.manifest
)

target_include_directories(QtLoginBrowser PRIVATE
    ${PROJECT_SOURCE_DIR}/src/sdologin
)

target_link_libraries(QtLoginBrowser PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    shell32
    winhttp
)

if(qtlogin_cef_osr_available)
    target_compile_definitions(QtLoginBrowser PRIVATE
        QTLOGIN_ENABLE_CEF_OSR
    )
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_compile_definitions(QtLoginBrowser PRIVATE
            "$<$<CONFIG:Debug>:_HAS_ITERATOR_DEBUGGING=0>"
            "$<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>"
        )
    endif()
    target_include_directories(QtLoginBrowser PRIVATE "${QTLOGIN_CEF109_ROOT}")
    target_link_libraries(QtLoginBrowser PRIVATE
        "$<$<CONFIG:Debug>:${QTLOGIN_CEF109_ROOT}/Debug/libcef.lib>"
        "$<$<NOT:$<CONFIG:Debug>>:${QTLOGIN_CEF109_ROOT}/Release/libcef.lib>"
        "$<$<CONFIG:Debug>:${qtlogin_cef_wrapper_root}/Debug/libcef_dll_wrapper.lib>"
        "$<$<NOT:$<CONFIG:Debug>>:${qtlogin_cef_wrapper_root}/Release/libcef_dll_wrapper.lib>"
    )
    add_custom_command(TARGET QtLoginBrowser POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<IF:$<CONFIG:Debug>,${QTLOGIN_CEF109_ROOT}/Debug,${QTLOGIN_CEF109_ROOT}/Release>"
            "$<TARGET_FILE_DIR:QtLoginBrowser>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QTLOGIN_CEF109_ROOT}/Resources"
            "$<TARGET_FILE_DIR:QtLoginBrowser>"
    )
    if(MSVC)
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            target_link_options(QtLoginBrowser PRIVATE "/STACK:0x8000")
        else()
            target_link_options(QtLoginBrowser PRIVATE "/STACK:0x800000")
        endif()
    endif()

    add_executable(CefDebugProbe cef_debug_probe.cpp)
    source_group("Source Files\\CEF调试工具" FILES
        cef_debug_probe.cpp
    )
    set_target_properties(CefDebugProbe PROPERTIES
        OUTPUT_NAME "cef_debug_probe"
        FOLDER "03_qtlogin_internal"
        PROJECT_LABEL "CefDebugProbe"
    )
    target_include_directories(CefDebugProbe PRIVATE "${QTLOGIN_CEF109_ROOT}")
    target_link_libraries(CefDebugProbe PRIVATE
        "$<$<CONFIG:Debug>:${QTLOGIN_CEF109_ROOT}/Debug/libcef.lib>"
        "$<$<NOT:$<CONFIG:Debug>>:${QTLOGIN_CEF109_ROOT}/Release/libcef.lib>"
        "$<$<CONFIG:Debug>:${qtlogin_cef_wrapper_root}/Debug/libcef_dll_wrapper.lib>"
        "$<$<NOT:$<CONFIG:Debug>>:${qtlogin_cef_wrapper_root}/Release/libcef_dll_wrapper.lib>"
    )
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_compile_definitions(CefDebugProbe PRIVATE
            "$<$<CONFIG:Debug>:_HAS_ITERATOR_DEBUGGING=0>"
            "$<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>"
        )
    endif()
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_link_options(CefDebugProbe PRIVATE "/STACK:0x8000")
    else()
        target_link_options(CefDebugProbe PRIVATE "/STACK:0x800000")
    endif()
    add_custom_command(TARGET CefDebugProbe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<IF:$<CONFIG:Debug>,${QTLOGIN_CEF109_ROOT}/Debug,${QTLOGIN_CEF109_ROOT}/Release>"
            "$<TARGET_FILE_DIR:CefDebugProbe>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QTLOGIN_CEF109_ROOT}/Resources"
            "$<TARGET_FILE_DIR:CefDebugProbe>"
    )
else()
    target_compile_definitions(QtLoginBrowser PRIVATE
        QTLOGIN_CEF_OSR_UNAVAILABLE_REASON="${qtlogin_cef_osr_unavailable_reason}"
    )
    message(STATUS "QtLogin browser CEF OSR disabled: ${qtlogin_cef_osr_unavailable_reason}")
endif()

if(MSVC)
    target_link_options(QtLoginBrowser PRIVATE
        "/MANIFEST:EMBED"
        "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/qtlogin_browser.exe.manifest"
    )
endif()

qtlogin_enable_windeployqt(QtLoginBrowser)
