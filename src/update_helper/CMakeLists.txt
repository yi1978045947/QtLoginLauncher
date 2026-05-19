add_executable(updatehelper
    main.cpp
)

source_group("Source Files\\更新辅助进程" FILES
    main.cpp
)

target_include_directories(updatehelper PRIVATE
    ${PROJECT_SOURCE_DIR}/src/common
)

target_link_libraries(updatehelper PRIVATE
    QtLoginCommon
    shell32
)

set_target_properties(updatehelper PROPERTIES
    OUTPUT_NAME "update"
)
