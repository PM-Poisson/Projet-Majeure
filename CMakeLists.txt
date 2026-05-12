cmake_minimum_required(VERSION 3.20)
project(CoastalErosion LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Dependances externes (placees dans libs/ comme git submodules)
# ---------------------------------------------------------------------------

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glfw)

# GLAD (fichiers generes a placer dans libs/glad/src/ et libs/glad/include/)
add_library(glad STATIC libs/glad/src/glad.c)
target_include_directories(glad PUBLIC libs/glad/include)

# GLM (header-only)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE libs/glm)

# Dear ImGui
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

# stb (header-only, juste besoin du include)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE libs/stb)

# ---------------------------------------------------------------------------
# Sources du projet
# ---------------------------------------------------------------------------

set(SOURCES
    src/main.cpp
    src/application.cpp

    src/terrain/bruit.cpp
    src/terrain/map.cpp
    src/terrain/terrain.cpp

    src/ocean/ocean.cpp
    src/ocean/wave.cpp

    src/erosion/erosion_system.cpp
    src/erosion/hydraulic_erosion.cpp

    src/renderer/renderer.cpp
    src/renderer/shader.cpp
    src/renderer/camera.cpp
    src/renderer/mesh.cpp

    src/UI/UI.cpp
)

add_executable(coastal-erosion ${SOURCES})

target_include_directories(coastal-erosion PRIVATE src)

target_link_libraries(coastal-erosion PRIVATE
    glfw
    glad
    glm
    imgui
    stb
)

# ---------------------------------------------------------------------------
# Copie des shaders dans le dossier de build (pour l'execution)
# ---------------------------------------------------------------------------

add_custom_command(TARGET coastal-erosion POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:coastal-erosion>/shaders
    COMMENT "Copie des shaders vers le dossier de build"
)

# ---------------------------------------------------------------------------
# Options de compilation
# ---------------------------------------------------------------------------

if(MSVC)
    target_compile_options(coastal-erosion PRIVATE /W4 /O2)
else()
    target_compile_options(coastal-erosion PRIVATE -Wall -Wextra -O2)
endif()