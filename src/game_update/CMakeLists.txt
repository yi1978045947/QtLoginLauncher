add_library(GameUpdate SHARED
    game_update.cpp
    game_update.def
)

source_group("Source Files\\更新SDK导出" FILES
    game_update.cpp
)
source_group("Export Files" FILES
    game_update.def
)

target_include_directories(GameUpdate PRIVATE
    ${PROJECT_SOURCE_DIR}/src/common
)

target_link_libraries(GameUpdate PRIVATE
    QtLoginCommon
)

set_target_properties(GameUpdate PROPERTIES
    OUTPUT_NAME "GameUpdate"
)
