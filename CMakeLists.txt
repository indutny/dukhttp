cmake_minimum_required(VERSION 3.4)
project(dukhttp LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Os -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")

# Include deps
add_subdirectory("deps/libuv" EXCLUDE_FROM_ALL)
add_subdirectory("deps/llhttp" EXCLUDE_FROM_ALL)
add_subdirectory("deps/duktape" EXCLUDE_FROM_ALL)

add_executable(dukhttp src/main.c)

add_dependencies(dukhttp uv_a duktape llhttp)

target_link_libraries(dukhttp uv_a duktape llhttp)
target_include_directories(dukhttp PUBLIC "${PROJECT_SOURCE_DIR}/src")
