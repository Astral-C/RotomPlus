cmake_minimum_required(VERSION 3.8)
project(Rotom)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(-DGLM_ENABLE_EXPERIMENTAL -DIMGUI_DEFINE_MATH_OPERATORS)

include_directories(lib/imgui lib/bstream)
add_subdirectory(lib)

file(GLOB_RECURSE ROTOM_SRC
    "src/*.cpp"
    "src/*.c"
    "include/*.hpp"
    "include/*.h"
    
    "lib/ImGuiFileDialog/ImGuiFileDialog/*.h"
    "lib/imgui/misc/cpp/*.cpp"
)

add_executable(rotom ${ROTOM_SRC})
target_include_directories(rotom PUBLIC include "${PROJECT_SOURCE_DIR}/lib" lib/ImGuiFileDialog/ImGuiFileDialog lib/glfw/include lib/palkia/include)

target_link_libraries(rotom PUBLIC palkia imgui ImGuiFileDialog glfw)
