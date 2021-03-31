cmake_minimum_required(VERSION 3.4)
project(dukhttp LANGUAGES C)

if(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Os -Wall -Wextra")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

# Include deps
add_subdirectory("deps/libuv" EXCLUDE_FROM_ALL)
add_subdirectory("deps/llhttp" EXCLUDE_FROM_ALL)
add_subdirectory("deps/duktape" EXCLUDE_FROM_ALL)

add_executable(dukhttp src/main.c)

add_dependencies(dukhttp uv_a duktape llhttp)

if(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
  target_link_libraries(dukhttp uv_a duktape llhttp m)
else()
  target_link_libraries(dukhttp uv_a duktape llhttp)
endif()

target_link_libraries(dukhttp uv_a duktape llhttp)
target_include_directories(dukhttp PUBLIC "${PROJECT_SOURCE_DIR}/src")
