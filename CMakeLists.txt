cmake_minimum_required(VERSION 3.15)
project(path_tracer)

set(CMAKE_C_STANDARD 23)

add_executable(${PROJECT_NAME} src/main.c)
target_include_directories(${PROJECT_NAME} PRIVATE include)
