set(sdologin_sources
    main.cpp
    account_history_model.cpp
    account_history_model.h
    command_line.cpp
    command_line.h
    code_key_login_model.cpp
    code_key_login_model.h
    dpi_scaler.cpp
    dpi_scaler.h
    legacy_password_auth.cpp
    legacy_password_auth.h
    legacy_password_crypto.cpp
    legacy_password_crypto.h
    protocol_acceptance_model.cpp
    protocol_acceptance_model.h
    push_message_login_model.cpp
    push_message_login_model.h
    sdo_base_push_message_auth.cpp
    sdo_base_push_message_auth.h
    sdo_base_code_key_auth.cpp
    sdo_base_code_key_auth.h
    sdo_base_password_auth.cpp
    sdo_base_password_auth.h
    virtual_keyboard_model.cpp
    virtual_keyboard_model.h
    window_embed_style.cpp
    window_embed_style.h
)

if(QTLOGIN_ENABLE_QT_UI)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
    include(${PROJECT_SOURCE_DIR}/cmake/QtDeploy.cmake)

    list(APPEND sdologin_sources
        account_history_combo.cpp
        account_history_combo.h
        login_window.cpp
        login_window.h
        code_key_login_panel.cpp
        code_key_login_panel.h
        protocol_browser_style.cpp
        protocol_browser_style.h
        push_message_login_panel.cpp
        push_message_login_panel.h
        quick_login_skin.cpp
        quick_login_skin.h
        virtual_keyboard_dialog.cpp
        virtual_keyboard_dialog.h
        protocol_browser_launcher.cpp
        protocol_browser_launcher.h
        qt_dpi_config.cpp
        qt_dpi_config.h
    )

    add_executable(sdologin WIN32 ${sdologin_sources})
    target_compile_definitions(sdologin PRIVATE QTLOGIN_ENABLE_QT_UI)
    target_link_libraries(sdologin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    set_target_properties(sdologin PROPERTIES AUTOMOC ON)
    if(MSVC)
        target_link_options(sdologin PRIVATE
            "/MANIFEST:EMBED"
            "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/sdologin.exe.manifest"
        )
    endif()
    qtlogin_enable_windeployqt(sdologin)
else()
    add_executable(sdologin ${sdologin_sources})
endif()

if(MSVC)
    target_compile_options(sdologin PRIVATE /utf-8)
endif()

source_group("Source Files\\入口与窗口" FILES
    main.cpp
    command_line.cpp
)
source_group("Header Files\\入口与窗口" FILES
    command_line.h
)

source_group("Source Files\\扫码登录" FILES
    code_key_login_model.cpp
    sdo_base_code_key_auth.cpp
)
source_group("Header Files\\扫码登录" FILES
    code_key_login_model.h
    sdo_base_code_key_auth.h
)

source_group("Source Files\\静密登录" FILES
    legacy_password_auth.cpp
    legacy_password_crypto.cpp
    sdo_base_password_auth.cpp
)
source_group("Header Files\\静密登录" FILES
    legacy_password_auth.h
    legacy_password_crypto.h
    sdo_base_password_auth.h
)

source_group("Source Files\\协议与Web" FILES
    protocol_acceptance_model.cpp
)
source_group("Header Files\\协议与Web" FILES
    protocol_acceptance_model.h
)

source_group("Source Files\\虚拟键盘" FILES
    virtual_keyboard_model.cpp
)
source_group("Header Files\\虚拟键盘" FILES
    virtual_keyboard_model.h
)

source_group("Source Files\\DPI与嵌入" FILES
    dpi_scaler.cpp
    window_embed_style.cpp
)
source_group("Header Files\\DPI与嵌入" FILES
    dpi_scaler.h
    window_embed_style.h
)

if(QTLOGIN_ENABLE_QT_UI)
    source_group("Source Files\\入口与窗口" FILES
        login_window.cpp
    )
    source_group("Header Files\\入口与窗口" FILES
        login_window.h
    )

    source_group("Source Files\\扫码登录" FILES
        code_key_login_panel.cpp
    )
    source_group("Header Files\\扫码登录" FILES
        code_key_login_panel.h
    )

    source_group("Source Files\\一键登录" FILES
        quick_login_skin.cpp
    )
    source_group("Header Files\\一键登录" FILES
        quick_login_skin.h
    )

    source_group("Source Files\\协议与Web" FILES
        protocol_browser_launcher.cpp
        protocol_browser_style.cpp
    )
    source_group("Header Files\\协议与Web" FILES
        protocol_browser_launcher.h
        protocol_browser_style.h
    )

    source_group("Source Files\\虚拟键盘" FILES
        virtual_keyboard_dialog.cpp
    )
    source_group("Header Files\\虚拟键盘" FILES
        virtual_keyboard_dialog.h
    )

    source_group("Source Files\\DPI与嵌入" FILES
        qt_dpi_config.cpp
    )
    source_group("Header Files\\DPI与嵌入" FILES
        qt_dpi_config.h
    )

    set_target_properties(sdologin PROPERTIES
        AUTOGEN_SOURCE_GROUP "Generated Files"
        AUTOMOC_SOURCE_GROUP "Generated Files"
    )
endif()

source_group("Source Files\\一键登录" FILES
    push_message_login_model.cpp
    sdo_base_push_message_auth.cpp
)
source_group("Header Files\\一键登录" FILES
    push_message_login_model.h
    sdo_base_push_message_auth.h
)

if(QTLOGIN_ENABLE_QT_UI)
    source_group("Source Files\\一键登录" FILES
        push_message_login_panel.cpp
        quick_login_skin.cpp
    )
    source_group("Header Files\\一键登录" FILES
        push_message_login_panel.h
        quick_login_skin.h
    )
endif()

if(QTLOGIN_ENABLE_QT_UI)
    source_group("Source Files\\Account History" FILES
        account_history_combo.cpp
    )
    source_group("Header Files\\Account History" FILES
        account_history_combo.h
    )
endif()

target_include_directories(sdologin PRIVATE
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/config
    ${PROJECT_SOURCE_DIR}/src/ipc
    ${PROJECT_SOURCE_DIR}/src/protocol
    ${PROJECT_SOURCE_DIR}/src/ui_model
)

set(qtlogin_sdobaseclient_root "${PROJECT_SOURCE_DIR}/third_party/sdologinclient")
if(QTLOGIN_LINK_LEGACY_AUTH_LIBS AND CMAKE_SIZEOF_VOID_P EQUAL 4 AND TARGET SdoLoginClient AND TARGET SafeStoreCryptoHelper)
    target_compile_definitions(sdologin PRIVATE QTLOGIN_ENABLE_LEGACY_AUTH_LIBS)
    target_include_directories(sdologin PRIVATE
        "${qtlogin_sdobaseclient_root}/libLoginClient"
    )
    target_link_libraries(sdologin PRIVATE
        SdoLoginClient
    )
    add_dependencies(sdologin
        SdoLoginClient
        SafeStoreCryptoHelper
    )
endif()

target_link_libraries(sdologin PRIVATE
    QtLoginCommon
    QtLoginConfig
    QtLoginIPC
    QtLoginProtocol
    QtLoginUIModel
    shell32
)

set_target_properties(sdologin PROPERTIES
    OUTPUT_NAME "sdologin"
)

if(EXISTS "${PROJECT_SOURCE_DIR}/assets/duilib_skin_reference")
    add_custom_command(TARGET sdologin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/assets/duilib_skin_reference"
            "$<TARGET_FILE_DIR:sdologin>/skin"
    )
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/assets/default_config")
    add_custom_command(TARGET sdologin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/assets/default_config"
            "$<TARGET_FILE_DIR:sdologin>"
    )
endif()
