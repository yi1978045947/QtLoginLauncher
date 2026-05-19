add_library(QtLoginCommon STATIC
    logger.cpp
    logger.h
    process_util.cpp
    process_util.h
    string_util.cpp
    string_util.h
)

source_group("Source Files\\日志" FILES
    logger.cpp
)
source_group("Header Files\\日志" FILES
    logger.h
)
source_group("Source Files\\进程工具" FILES
    process_util.cpp
)
source_group("Header Files\\进程工具" FILES
    process_util.h
)
source_group("Source Files\\字符串工具" FILES
    string_util.cpp
)
source_group("Header Files\\字符串工具" FILES
    string_util.h
)

target_include_directories(QtLoginCommon PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
