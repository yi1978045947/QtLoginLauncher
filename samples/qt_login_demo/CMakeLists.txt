if(NOT QTLOGIN_ENABLE_QT_UI)
    return()
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
include(${PROJECT_SOURCE_DIR}/cmake/QtDeploy.cmake)

add_executable(QtLoginDemo WIN32
    main.cpp
)

source_group("Source Files\\Demo入口" FILES
    main.cpp
)

add_dependencies(QtLoginDemo
    sdk
    sdologin
    QtLoginBrowser
)

target_include_directories(QtLoginDemo PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(QtLoginDemo PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    oleaut32
)

if(MSVC)
    target_compile_options(QtLoginDemo PRIVATE /utf-8)
endif()

set_target_properties(QtLoginDemo PROPERTIES
    AUTOMOC ON
    OUTPUT_NAME "qt_login_demo"
)

qtlogin_enable_windeployqt(QtLoginDemo)
