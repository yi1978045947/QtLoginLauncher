add_library(QtLoginProtocol STATIC
    json_payload.cpp
    json_payload.h
    protocol_frame.cpp
    protocol_frame.h
    protocol_types.h
)

source_group("Source Files\\JSON载荷" FILES
    json_payload.cpp
)
source_group("Header Files\\JSON载荷" FILES
    json_payload.h
)
source_group("Source Files\\协议帧" FILES
    protocol_frame.cpp
)
source_group("Header Files\\协议帧" FILES
    protocol_frame.h
)
source_group("Header Files\\协议类型" FILES
    protocol_types.h
)

target_include_directories(QtLoginProtocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
