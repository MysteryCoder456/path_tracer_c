cmake_minimum_required(VERSION 3.15)
project(path_tracer)
set(CMAKE_C_STANDARD 23)

add_executable(${PROJECT_NAME} src/main.c src/scene.c src/ray.c src/bitmap.c src/threadpool.c src/vector.c)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE external)

if (UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif (UNIX)

target_link_libraries(${PROJECT_NAME} PRIVATE cglm_headers)
add_subdirectory(external/cglm EXCLUDE_FROM_ALL)
