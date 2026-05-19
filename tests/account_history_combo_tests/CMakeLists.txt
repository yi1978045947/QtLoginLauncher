if(NOT QTLOGIN_ENABLE_QT_UI)
    return()
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(account_history_combo_tests
    main.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/account_history_combo.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/account_history_combo.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/account_history_model.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/account_history_model.h
)

target_include_directories(account_history_combo_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/sdologin
    ${PROJECT_SOURCE_DIR}/src/config
)

target_link_libraries(account_history_combo_tests PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
)

set_target_properties(account_history_combo_tests PROPERTIES
    AUTOMOC ON
    FOLDER "05_tests"
    PROJECT_LABEL "account_history_combo_tests"
)

source_group("Source Files\\测试入口" FILES
    main.cpp
)
source_group("Source Files\\被测代码\\Account History" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/account_history_combo.cpp
    ${PROJECT_SOURCE_DIR}/src/sdologin/account_history_model.cpp
)
source_group("Header Files\\被测代码\\Account History" FILES
    ${PROJECT_SOURCE_DIR}/src/sdologin/account_history_combo.h
    ${PROJECT_SOURCE_DIR}/src/sdologin/account_history_model.h
)
