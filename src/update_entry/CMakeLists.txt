add_library(update_entry SHARED
    update_entry.cpp
    update_entry.def
)

source_group("Source Files\\更新入口" FILES
    update_entry.cpp
)
source_group("Export Files" FILES
    update_entry.def
)

target_include_directories(update_entry PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
)

target_link_libraries(update_entry PRIVATE
    QtLoginCommon
    ole32
)

set_target_properties(update_entry PROPERTIES
    OUTPUT_NAME "sdologinentry"
)
