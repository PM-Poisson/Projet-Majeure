cmake_minimum_required(VERSION 3.20)
project(CoastalErosion LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glfw)

add_library(glad STATIC libs/glad/src/glad.c)
target_include_directories(glad PUBLIC libs/glad/include)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE libs/glm)

set(IMGUI_DIR libs/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE libs/stb)

set(SOURCES
    src/main.cpp

    src/terrain/bruit.cpp
    src/terrain/map.cpp
    src/terrain/terrain.cpp
    src/terrain/scene.cpp

    src/ocean/ocean.cpp

    src/erosion/erosion_system.cpp
    src/erosion/hydraulic_erosion.cpp

    src/renderer/shader.cpp
    src/renderer/camera.cpp
    src/renderer/mesh.cpp

    src/external/perlin/simplexnoise1234.cpp
)

add_executable(coastal-erosion ${SOURCES})
target_include_directories(coastal-erosion PRIVATE src)
target_link_libraries(coastal-erosion PRIVATE glfw glad glm imgui stb)

add_custom_command(TARGET coastal-erosion POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:coastal-erosion>/shaders
    COMMENT "Copie des shaders"
)

if(MSVC)
    target_compile_options(coastal-erosion PRIVATE /W4 /O2)
else()
    target_compile_options(coastal-erosion PRIVATE -Wall -Wextra -O2)
endif()