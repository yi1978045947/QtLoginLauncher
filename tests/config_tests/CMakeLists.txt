add_executable(config_tests
    main.cpp
)

source_group("Source Files\\测试入口" FILES
    main.cpp
)

target_link_libraries(config_tests PRIVATE
    QtLoginConfig
)
