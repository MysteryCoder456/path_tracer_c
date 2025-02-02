cmake_minimum_required(VERSION 3.15)
project(path_tracer)
set(CMAKE_C_STANDARD 23)

add_subdirectory(external/glfw)

add_executable(${PROJECT_NAME} src/main.c src/renderer.c src/scene.c src/ray.c src/bitmap.c src/threadpool.c src/vector.c external/glad/src/gl.c)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE external)
target_include_directories(${PROJECT_NAME} PRIVATE external/glad/include)

if (UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif (UNIX)

target_link_libraries(${PROJECT_NAME} PRIVATE cglm_headers)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
add_subdirectory(external/cglm EXCLUDE_FROM_ALL)

# Copy shaders
configure_file(shaders/triangle_vert.glsl shaders/triangle_vert.glsl COPYONLY)
configure_file(shaders/triangle_frag.glsl shaders/triangle_frag.glsl COPYONLY)
