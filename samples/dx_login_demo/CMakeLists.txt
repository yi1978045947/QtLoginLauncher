add_executable(DXLoginDemo WIN32
    main.cpp
    dx_demo_behavior.cpp
    dx_demo_behavior.h
)

source_group("Source Files\\Demo入口" FILES
    main.cpp
)
source_group("Source Files\\DX渲染与SDK调用" FILES
    dx_demo_behavior.cpp
)
source_group("Header Files\\DX渲染与SDK调用" FILES
    dx_demo_behavior.h
)

target_include_directories(DXLoginDemo PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(DXLoginDemo PRIVATE
    d3d9
    oleaut32
)

add_dependencies(DXLoginDemo
    sdk
    sdologin
)

set_target_properties(DXLoginDemo PROPERTIES
    OUTPUT_NAME "dx_login_demo"
)

if(EXISTS "${PROJECT_SOURCE_DIR}/assets/default_config")
    add_custom_command(TARGET DXLoginDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/assets/default_config"
            "$<TARGET_FILE_DIR:DXLoginDemo>"
    )
endif()
