add_executable(sdk_module_tests
    main.cpp
)

source_group("Source Files\\测试入口" FILES
    main.cpp
)

target_include_directories(sdk_module_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(sdk_module_tests PRIVATE
    oleaut32
)

if(TARGET SafeStoreCryptoHelper)
    add_dependencies(sdk_module_tests SafeStoreCryptoHelper)
endif()
