cmake_minimum_required(VERSION 3.15)

project(Graphic-Homework C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenGL REQUIRED)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libraries)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
    ${LIB_DIR}/glm
)

add_library(glfw_imported STATIC IMPORTED)
set_target_properties(glfw_imported PROPERTIES
    IMPORTED_LOCATION "${LIB_DIR}/glfw/lib-vc2022/glfw3.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${LIB_DIR}/glfw/include"
)

add_library(glad STATIC
    ${LIB_DIR}/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    ${LIB_DIR}/glad/include
)

target_link_libraries(glad PRIVATE OpenGL::GL)


set(IMGUI_DIR ${LIB_DIR}/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    glad
    glfw_imported
    glm
)

add_executable(Graphic-Homework main.cpp)

target_link_libraries(Graphic-Homework PRIVATE imgui)