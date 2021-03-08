cmake_minimum_required(VERSION 3.4)
project(duktape LANGUAGES C)

add_library(duktape src/duktape.c)

target_include_directories(duktape PUBLIC "${PROJECT_SOURCE_DIR}/src")
