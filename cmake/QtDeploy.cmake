function(qtlogin_enable_windeployqt target_name)
    if(NOT WIN32)
        return()
    endif()

    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "qtlogin_enable_windeployqt target does not exist: ${target_name}")
    endif()

    if(NOT DEFINED QT_VERSION_MAJOR OR NOT TARGET Qt${QT_VERSION_MAJOR}::qmake)
        message(WARNING "Qt qmake target was not found; skipping Qt runtime deployment for ${target_name}.")
        return()
    endif()

    get_target_property(qtlogin_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    if(NOT qtlogin_qmake_executable)
        message(WARNING "Qt qmake executable was not found; skipping Qt runtime deployment for ${target_name}.")
        return()
    endif()

    get_filename_component(qtlogin_qt_bin_dir "${qtlogin_qmake_executable}" DIRECTORY)
    find_program(qtlogin_windeployqt_executable
        NAMES windeployqt.exe windeployqt
        HINTS "${qtlogin_qt_bin_dir}"
        NO_DEFAULT_PATH
    )
    if(NOT qtlogin_windeployqt_executable)
        find_program(qtlogin_windeployqt_executable NAMES windeployqt.exe windeployqt)
    endif()

    if(NOT qtlogin_windeployqt_executable)
        message(WARNING "windeployqt was not found; skipping Qt runtime deployment for ${target_name}.")
        return()
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND "${qtlogin_windeployqt_executable}"
            "$<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>"
            --no-translations
            --no-compiler-runtime
            "$<TARGET_FILE:${target_name}>"
        COMMENT "Deploying Qt runtime for ${target_name}"
        VERBATIM
    )
endfunction()
