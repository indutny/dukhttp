cmake_minimum_required(VERSION 3.4)
project(llhttp LANGUAGES C)

add_library(llhttp src/api.c src/http.c src/llhttp.c)

target_include_directories(llhttp
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
