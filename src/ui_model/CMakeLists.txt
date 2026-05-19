add_library(QtLoginUIModel STATIC
    legacy_dialog_registry.cpp
    legacy_dialog_registry.h
    ui_model.h
)

source_group("Source Files\\旧窗口映射" FILES
    legacy_dialog_registry.cpp
)
source_group("Header Files\\旧窗口映射" FILES
    legacy_dialog_registry.h
)
source_group("Header Files\\UI模型" FILES
    ui_model.h
)

target_include_directories(QtLoginUIModel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
