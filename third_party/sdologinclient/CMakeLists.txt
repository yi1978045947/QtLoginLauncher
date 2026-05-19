if(NOT WIN32)
    return()
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "SdoBaseClient is Win32-only in the imported sdologinclient project; skipping for this build.")
    return()
endif()

enable_language(C)

set(SDOLOGINCLIENT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(SDOLOGINCLIENT_RUNTIME_DIR "${SDOLOGINCLIENT_ROOT}/output/Release")

function(sdobase_apply_legacy_compile_settings target_name)
    target_compile_definitions(${target_name} PRIVATE
        WIN32
        NDEBUG
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )
    if(MSVC)
        set_target_properties(${target_name} PROPERTIES
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED ON
            MSVC_RUNTIME_LIBRARY "MultiThreaded"
        )
        target_compile_options(${target_name} PRIVATE
            /wd4005
            /wd4018
            /wd4244
            /wd4267
            /wd4996
        )
    endif()
endfunction()

set(SDOLOGINCLIENT_COMMON_INCLUDE_DIRS
    "${SDOLOGINCLIENT_ROOT}"
    "${SDOLOGINCLIENT_ROOT}/include"
    "${SDOLOGINCLIENT_ROOT}/include/curl_include/include"
    "${SDOLOGINCLIENT_ROOT}/include/openssl_include"
    "${SDOLOGINCLIENT_ROOT}/libUtil"
    "${SDOLOGINCLIENT_ROOT}/libLoginClient"
)

add_library(libUtil STATIC
    libUtil/json/arraylist.c
    libUtil/json/debug.c
    libUtil/json/json_object.c
    libUtil/json/json_tokener.c
    libUtil/json/json_util.c
    libUtil/json/linkhash.c
    libUtil/json/printbuf.c
    libUtil/md5.cpp
)
source_group("Source Files\\JSON" FILES
    libUtil/json/arraylist.c
    libUtil/json/debug.c
    libUtil/json/json_object.c
    libUtil/json/json_tokener.c
    libUtil/json/json_util.c
    libUtil/json/linkhash.c
    libUtil/json/printbuf.c
)
source_group("Source Files\\MD5" FILES
    libUtil/md5.cpp
)
target_include_directories(libUtil PUBLIC
    "${SDOLOGINCLIENT_ROOT}"
    "${SDOLOGINCLIENT_ROOT}/libUtil"
)
sdobase_apply_legacy_compile_settings(libUtil)

add_library(libLoginClient STATIC
    include/LicenseManager.cpp
    include/PathHelper.cpp
    include/Tracer.cpp
    libLoginClient/CommonFun.cpp
    libLoginClient/ComputerInfo.cpp
    libLoginClient/CurlHttpClient.cpp
    libLoginClient/CurlHttpDownload.cpp
    libLoginClient/CurlHttpThread.cpp
    libLoginClient/GunionCrypto.cpp
    libLoginClient/GunionHandshake.cpp
    libLoginClient/HttpClient.cpp
    libLoginClient/HttpThread.cpp
    libLoginClient/LoginClient.cpp
    libLoginClient/MsgProcess.cpp
    libLoginClient/SafeBase64.cpp
    libLoginClient/SdoBaseClient.cpp
    libLoginClient/stdafx.cpp
    libLoginClient/WinHttpClient.cpp
)
source_group("Source Files\\基础工具" FILES
    include/LicenseManager.cpp
    include/PathHelper.cpp
    include/Tracer.cpp
    libLoginClient/CommonFun.cpp
    libLoginClient/ComputerInfo.cpp
    libLoginClient/stdafx.cpp
)
source_group("Source Files\\HTTP通信" FILES
    libLoginClient/CurlHttpClient.cpp
    libLoginClient/CurlHttpDownload.cpp
    libLoginClient/CurlHttpThread.cpp
    libLoginClient/HttpClient.cpp
    libLoginClient/HttpThread.cpp
    libLoginClient/WinHttpClient.cpp
)
source_group("Source Files\\加密与握手" FILES
    libLoginClient/GunionCrypto.cpp
    libLoginClient/GunionHandshake.cpp
    libLoginClient/SafeBase64.cpp
)
source_group("Source Files\\登录客户端" FILES
    libLoginClient/LoginClient.cpp
    libLoginClient/MsgProcess.cpp
    libLoginClient/SdoBaseClient.cpp
)
target_include_directories(libLoginClient PUBLIC
    ${SDOLOGINCLIENT_COMMON_INCLUDE_DIRS}
)
target_link_libraries(libLoginClient PUBLIC
    libUtil
)
sdobase_apply_legacy_compile_settings(libLoginClient)

add_library(SdoLoginClient SHARED
    SdoLoginClient/dllmain.cpp
    SdoLoginClient/stdafx.cpp
)
source_group("Source Files\\DLL入口" FILES
    SdoLoginClient/dllmain.cpp
    SdoLoginClient/stdafx.cpp
)
target_include_directories(SdoLoginClient PRIVATE
    ${SDOLOGINCLIENT_COMMON_INCLUDE_DIRS}
    "${SDOLOGINCLIENT_ROOT}/SdoLoginClient"
)
target_link_libraries(SdoLoginClient PRIVATE
    libLoginClient
    "${SDOLOGINCLIENT_RUNTIME_DIR}/libcurl.lib"
    "${SDOLOGINCLIENT_RUNTIME_DIR}/libcurl_imp.lib"
    "${SDOLOGINCLIENT_RUNTIME_DIR}/libcrypto.lib"
    "${SDOLOGINCLIENT_ROOT}/libUtil/SafeStore.lib"
    wininet
    ws2_32
    ole32
    shell32
    Advapi32
    user32
    setupapi
    Iphlpapi
    winhttp
    Crypt32
)
sdobase_apply_legacy_compile_settings(SdoLoginClient)
set_target_properties(SdoLoginClient PROPERTIES
    OUTPUT_NAME "SdoBaseClient"
)
if(MSVC)
    target_link_options(SdoLoginClient PRIVATE
        "/DEF:${SDOLOGINCLIENT_ROOT}/SdoLoginClient/SdoLoginClient.def"
    )
endif()

foreach(runtime_file
    libcurl.dll
    libssl-1_1.dll
    libcrypto-1_1.dll
    cacert.pem
    ver.bin
)
    if(EXISTS "${SDOLOGINCLIENT_RUNTIME_DIR}/${runtime_file}")
        add_custom_command(TARGET SdoLoginClient POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SDOLOGINCLIENT_RUNTIME_DIR}/${runtime_file}"
                "$<TARGET_FILE_DIR:SdoLoginClient>/${runtime_file}"
        )
    endif()
endforeach()

add_library(SafeStoreCryptoHelper SHARED
    qtlogin_sdobase_crypto.cpp
)
source_group("Source Files\\加密导出" FILES
    qtlogin_sdobase_crypto.cpp
)
target_include_directories(SafeStoreCryptoHelper PRIVATE
    "${SDOLOGINCLIENT_ROOT}"
)
target_link_libraries(SafeStoreCryptoHelper PRIVATE
    "${SDOLOGINCLIENT_ROOT}/libUtil/SafeStore.lib"
    Crypt32
)
sdobase_apply_legacy_compile_settings(SafeStoreCryptoHelper)
set_target_properties(SafeStoreCryptoHelper PROPERTIES
    OUTPUT_NAME "qtlogin_sdobase_crypto"
)
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_link_options(SafeStoreCryptoHelper PRIVATE
        "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/qtlogin_sdobase_crypto.def"
    )
endif()
