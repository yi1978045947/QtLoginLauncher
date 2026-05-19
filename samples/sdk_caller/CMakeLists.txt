add_executable(SDKCallerDemo
    main.cpp
)

source_group("Source Files\\SDK调用示例" FILES
    main.cpp
)

target_include_directories(SDKCallerDemo PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(SDKCallerDemo PRIVATE
    oleaut32
)

set_target_properties(SDKCallerDemo PROPERTIES
    OUTPUT_NAME "sdk_caller"
)
