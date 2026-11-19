cmake_minimum_required(VERSION 3.16)
project(fire_alarm_host_tests LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_testing()

set(TEST_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/lib_fusion/include"
)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)

function(host_test_common target)
    target_include_directories(${target} PRIVATE ${TEST_INCLUDE_DIRS})
    if(ENABLE_SANITIZERS)
        if(MSVC)
            message(FATAL_ERROR "ENABLE_SANITIZERS is not supported with MSVC")
        endif()
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${target} PRIVATE
            -fsanitize=address,undefined
        )
    endif()
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if(MINGW OR (UNIX AND NOT APPLE))
            target_link_libraries(${target} PRIVATE m)
        endif()
    endif()
endfunction()

add_library(proto_ndjson STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/common/proto_ndjson.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/common/proto_ndjson_json.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/common/proto_ndjson_dec.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/common/proto_ndjson_enc.c"
)
target_include_directories(proto_ndjson PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/common"
)
host_test_common(proto_ndjson)

add_executable(test_sanity test_sanity.c)
host_test_common(test_sanity)

foreach(t proto_enc proto_dec proto_dec_err proto_asm)
    add_executable(test_${t} test_${t}.c)
    target_link_libraries(test_${t} PRIVATE proto_ndjson)
    host_test_common(test_${t})
    add_test(NAME ProtoNdjson_${t} COMMAND test_${t})
endforeach()

add_test(NAME SanityCheck COMMAND test_sanity)
