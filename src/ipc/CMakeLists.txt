add_library(QtLoginIPC STATIC
    named_pipe.cpp
    named_pipe.h
)

source_group("Source Files\\命名管道" FILES
    named_pipe.cpp
)
source_group("Header Files\\命名管道" FILES
    named_pipe.h
)

target_include_directories(QtLoginIPC PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(QtLoginIPC PUBLIC
    QtLoginProtocol
)
