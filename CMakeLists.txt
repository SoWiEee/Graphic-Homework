cmake_minimum_required(VERSION 3.15)

project(Graphic-Homework C CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)

include_directories(
    libraries/
    libraries/imgui
    libraries/imgui/backends
    libraries/glm
    libraries/glad/include
    libraries/glfw/include
)

set(GLFW_LIB ${CMAKE_CURRENT_SOURCE_DIR}/libraries/glfw/lib-vc2022/glfw3.lib)
set(LINKED_LIBS opengl32 ${GLFW_LIB})
 
set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libraries/glad)
set(GLAD_SOURCE_FILES
    ${GLAD_DIR}/src/glad.c
)
add_library(glad ${GLAD_SOURCE_FILES})
 
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libraries/imgui)
set(IMGUI_SOURCE_FILES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui ${IMGUI_SOURCE_FILES})

add_executable(Graphic-Homework main.cpp)
target_link_libraries(Graphic-Homework ${LINKED_LIBS} imgui glad)