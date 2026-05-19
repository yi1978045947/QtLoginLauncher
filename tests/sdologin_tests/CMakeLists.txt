add_executable(sdologin_tests
    main.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/command_line.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/command_line.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/account_history_model.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/account_history_model.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/code_key_login_model.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/code_key_login_model.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/cef_web_bridge.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/cef_web_bridge.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/dpi_scaler.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/dpi_scaler.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/legacy_password_auth.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/legacy_password_auth.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_acceptance_model.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_acceptance_model.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_launcher.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_launcher.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_style.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_style.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/push_message_login_model.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/push_message_login_model.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/quick_login_skin.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/quick_login_skin.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/virtual_keyboard_model.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/virtual_keyboard_model.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/window_embed_style.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/window_embed_style.h
    ${PROJECT_SOURCE_DIR}/samples/dx_login_demo/dx_demo_behavior.cpp
    ${PROJECT_SOURCE_DIR}/samples/dx_login_demo/dx_demo_behavior.h
)

source_group("Source Files\\测试入口" FILES
    main.cpp
)
source_group("Source Files\\被测代码\\入口与命令行" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/command_line.cpp
)
source_group("Header Files\\被测代码\\入口与命令行" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/command_line.h
)
source_group("Source Files\\被测代码\\扫码登录" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/code_key_login_model.cpp
)
source_group("Header Files\\被测代码\\扫码登录" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/code_key_login_model.h
)
source_group("Source Files\\被测代码\\静密登录" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/legacy_password_auth.cpp
)
source_group("Header Files\\被测代码\\静密登录" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/legacy_password_auth.h
)
source_group("Source Files\\被测代码\\协议与Web" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/cef_web_bridge.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_acceptance_model.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_launcher.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_style.cpp
)
source_group("Header Files\\被测代码\\协议与Web" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/cef_web_bridge.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_acceptance_model.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_launcher.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/protocol_browser_style.h
)
source_group("Source Files\\被测代码\\一键登录" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/quick_login_skin.cpp
)
source_group("Header Files\\被测代码\\一键登录" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/quick_login_skin.h
)
source_group("Source Files\\被测代码\\虚拟键盘" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/virtual_keyboard_model.cpp
)
source_group("Header Files\\被测代码\\虚拟键盘" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/virtual_keyboard_model.h
)
source_group("Source Files\\被测代码\\DPI与嵌入" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/dpi_scaler.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/window_embed_style.cpp
)
source_group("Header Files\\被测代码\\DPI与嵌入" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/dpi_scaler.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/window_embed_style.h
)
source_group("Source Files\\被测代码\\DX Demo行为" FILES
    ${PROJECT_SOURCE_DIR}/samples/dx_login_demo/dx_demo_behavior.cpp
)
source_group("Header Files\\被测代码\\DX Demo行为" FILES
    ${PROJECT_SOURCE_DIR}/samples/dx_login_demo/dx_demo_behavior.h
)

target_include_directories(sdologin_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/sdologin
    ${PROJECT_SOURCE_DIR}/src/config
    ${PROJECT_SOURCE_DIR}/samples/dx_login_demo
)

target_link_libraries(sdologin_tests PRIVATE
    QtLoginCommon
)
