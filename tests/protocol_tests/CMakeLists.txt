add_executable(protocol_tests
    main.cpp
)

source_group("Source Files\\测试入口" FILES
    main.cpp
)

target_link_libraries(protocol_tests PRIVATE
    QtLoginProtocol
)
