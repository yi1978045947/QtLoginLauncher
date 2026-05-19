add_library(QtLoginConfig STATIC
    client_config_model.cpp
    client_config_model.h
    config_manager.cpp
    config_manager.h
)

source_group("Source Files\\配置模型" FILES
    client_config_model.cpp
)
source_group("Header Files\\配置模型" FILES
    client_config_model.h
)
source_group("Source Files\\配置读取" FILES
    config_manager.cpp
)
source_group("Header Files\\配置读取" FILES
    config_manager.h
)

target_include_directories(QtLoginConfig PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(QtLoginConfig PUBLIC
    QtLoginCommon
)
