cmake_minimum_required(VERSION 3.16)
project(MiniSpireSFML LANGUAGES CXX)

include(CTest)
include(CheckIPOSupported)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_SYSTEM_SFML "Use system-installed SFML instead of FetchContent" OFF)
option(BUILD_SELF_TEST "Build MiniSpireSelfTest executable and register ctest" ON)
option(ENABLE_LTO "Enable IPO/LTO where supported" OFF)
option(ENABLE_AUDIO "Enable SFML audio module and in-game BGM/SFX" OFF)

if (NOT USE_SYSTEM_SFML)
    include(FetchContent)

    FetchContent_Declare(
        sfml
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.1
    )

    if (ENABLE_AUDIO)
        set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
    else()
        set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    endif()
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    if (ENABLE_AUDIO)
        set(SFML_DISABLE_FLAC ON CACHE BOOL "" FORCE)
        set(SFML_DISABLE_MP3 OFF CACHE BOOL "" FORCE)
    endif()

    FetchContent_MakeAvailable(sfml)
else()
    if (ENABLE_AUDIO)
        find_package(SFML 2.6 REQUIRED COMPONENTS graphics window system audio)
    else()
        find_package(SFML 2.6 REQUIRED COMPONENTS graphics window system)
    endif()
endif()

if (ENABLE_AUDIO AND NOT MSVC)
    set(MINGW_COMPAT_SRC src/mingw_compat.cpp)
else()
    set(MINGW_COMPAT_SRC "")
endif()

add_executable(MiniSpireSFML
    src/main.cpp
    ${MINGW_COMPAT_SRC}
)

if (BUILD_SELF_TEST)
    add_executable(MiniSpireSelfTest
        src/main.cpp
        ${MINGW_COMPAT_SRC}
    )
endif()

target_link_libraries(MiniSpireSFML PRIVATE sfml-graphics sfml-window sfml-system)
if (BUILD_SELF_TEST)
    target_link_libraries(MiniSpireSelfTest PRIVATE sfml-graphics sfml-window sfml-system)
endif()

if (ENABLE_AUDIO)
    target_compile_definitions(MiniSpireSFML PRIVATE USE_AUDIO=1)
    target_link_libraries(MiniSpireSFML PRIVATE sfml-audio)
    if (BUILD_SELF_TEST)
        target_compile_definitions(MiniSpireSelfTest PRIVATE USE_AUDIO=1)
        target_link_libraries(MiniSpireSelfTest PRIVATE sfml-audio)
    endif()
else()
    target_compile_definitions(MiniSpireSFML PRIVATE USE_AUDIO=0)
    if (BUILD_SELF_TEST)
        target_compile_definitions(MiniSpireSelfTest PRIVATE USE_AUDIO=0)
    endif()
endif()

get_filename_component(CXX_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

# Copy SFML runtime files beside the exe so launching from the build folder works.
if (NOT USE_SYSTEM_SFML)
    add_custom_command(TARGET MiniSpireSFML POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_BINARY_DIR}/_deps/sfml-build/lib"
        "$<TARGET_FILE_DIR:MiniSpireSFML>"
    )
    if (ENABLE_AUDIO)
        add_custom_command(TARGET MiniSpireSFML POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/_deps/sfml-src/extlibs/bin/x64/openal32.dll"
            "$<TARGET_FILE_DIR:MiniSpireSFML>"
        )
    endif()
    if (BUILD_SELF_TEST)
        add_custom_command(TARGET MiniSpireSelfTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/_deps/sfml-build/lib"
            "$<TARGET_FILE_DIR:MiniSpireSelfTest>"
        )
        if (ENABLE_AUDIO)
            add_custom_command(TARGET MiniSpireSelfTest POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/_deps/sfml-src/extlibs/bin/x64/openal32.dll"
                "$<TARGET_FILE_DIR:MiniSpireSelfTest>"
            )
        endif()
    endif()
endif()

if (MSVC)
    target_compile_options(MiniSpireSFML PRIVATE /W4 /permissive- /utf-8)
    if (BUILD_SELF_TEST)
        target_compile_options(MiniSpireSelfTest PRIVATE /W4 /permissive- /utf-8)
    endif()
else()
    target_compile_options(MiniSpireSFML PRIVATE -Wall -Wextra -Wpedantic)
    if (BUILD_SELF_TEST)
        target_compile_options(MiniSpireSelfTest PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if (ENABLE_LTO)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if (ipo_supported)
        set_property(TARGET MiniSpireSFML PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        if (BUILD_SELF_TEST)
            set_property(TARGET MiniSpireSelfTest PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        endif()
    else()
        message(WARNING "LTO requested but not supported: ${ipo_error}")
    endif()
endif()

if (BUILD_TESTING AND BUILD_SELF_TEST)
    add_test(NAME smoke_self_test COMMAND MiniSpireSelfTest --self-test)
    if (USE_SYSTEM_SFML)
        set_tests_properties(smoke_self_test PROPERTIES
            WORKING_DIRECTORY $<TARGET_FILE_DIR:MiniSpireSelfTest>
            ENVIRONMENT "PATH=${CXX_COMPILER_DIR};$ENV{PATH}"
        )
    else()
        set_tests_properties(smoke_self_test PROPERTIES
            WORKING_DIRECTORY $<TARGET_FILE_DIR:MiniSpireSelfTest>
            ENVIRONMENT "PATH=${CXX_COMPILER_DIR};${CMAKE_BINARY_DIR}/_deps/sfml-build/lib;$ENV{PATH}"
        )
    endif()
endif()
