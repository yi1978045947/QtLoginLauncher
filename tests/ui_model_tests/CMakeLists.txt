add_executable(ui_model_tests
    main.cpp
)

source_group("Source Files\\测试入口" FILES
    main.cpp
)

target_link_libraries(ui_model_tests PRIVATE
    QtLoginUIModel
)
