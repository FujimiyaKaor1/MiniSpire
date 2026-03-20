cmake_minimum_required(VERSION 3.16)
project(MiniSpireSFML LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1
)

set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sfml)

add_executable(MiniSpireSFML
    src/main.cpp
)

target_link_libraries(MiniSpireSFML PRIVATE sfml-graphics sfml-window sfml-system)

# Copy SFML runtime files beside the exe so launching from the build folder works.
add_custom_command(TARGET MiniSpireSFML POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_BINARY_DIR}/_deps/sfml-build/lib"
    "$<TARGET_FILE_DIR:MiniSpireSFML>"
)

if (MSVC)
    target_compile_options(MiniSpireSFML PRIVATE /W4 /permissive-)
else()
    target_compile_options(MiniSpireSFML PRIVATE -Wall -Wextra -Wpedantic)
endif()
