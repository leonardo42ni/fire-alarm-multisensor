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

add_executable(test_sanity test_sanity.c)
target_include_directories(test_sanity PRIVATE ${TEST_INCLUDE_DIRS})

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)

if(ENABLE_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "ENABLE_SANITIZERS is not supported with MSVC")
    endif()
    target_compile_options(test_sanity PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(test_sanity PRIVATE
        -fsanitize=address,undefined
    )
endif()

if(MSVC)
    target_compile_options(test_sanity PRIVATE /W4)
else()
    target_compile_options(test_sanity PRIVATE -Wall -Wextra)
    if(MINGW OR (UNIX AND NOT APPLE))
        target_link_libraries(test_sanity PRIVATE m)
    endif()
endif()

add_test(NAME SanityCheck COMMAND test_sanity)
