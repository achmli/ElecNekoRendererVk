cmake_minimum_required(VERSION 3.28)
project(ElceNekoSurface)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SHADERS
        ${CMAKE_SOUURCE_DIR}/src/shaders/*.glsl
        ${CMAKE_SOUURCE_DIR}/src/shaders/*.hlsl
)

file(GLOB_RECURSE SRC_FILES
        ${CMAKE_SOURCE_DIR}/src/*.h
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/thirdParty/xatlas/xatlas.cpp
)

set(SRCS ${SRC_FILES} ${SHADERS})

include_directories(
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/thirdParty/stb
        ${CMAKE_SOURCE_DIR}/thirdpParty/tinyobjloader
        ${CMAKE_SOURCE_DIR}/thirdParty/eigen
        ${CMAKE_SOURCE_DIR}/thirdParty/xatlas
)

add_subdirectory(${CMAKE_SOURCE_DIR}/thirdParty/glfw)
include_directories(${CMAKE_SOURCE_DIR}/thirdParty/glfw/include)

add_library(imgui STATIC
        ${CMAKE_SOURCE_DIR}/thirdParty/imgui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/thirdParty/imgui/imgui_demo.cpp
        ${CMAKE_SOURCE_DIR}/thirdParty/imgui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/thirdParty/imgui/imgui_tables.cpp
        ${CMAKE_SOURCE_DIR}/thirdParty/imgui/imgui_widgets.cpp
        ${CMAKE_SOURCE_DIR}/thirdParty/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/thirdParty/imgui/backends/imgui_impl_vulkan.cpp
)
include_directories(${CMAKE_SOURCE_DIR}/thirdParty/imgui)
include_directories(${CMAKE_SOURCE_DIR}/thirdParty/imgui/backends)

# 添加Vulkan头文件路径
include_directories(${CMAKE_SOURCE_DIR}/thirdParty/vulkan/Include)

# 查找Vulkan库文件
find_library(VULKAN_LIBRARY
        NAMES vulkan-1 vulkan
        PATHS ${CMAKE_SOURCE_DIR}/thirdParty/vulkan/Lib
)

add_executable(ElceNekoSurface ${SRCS})

target_link_libraries(ElceNekoSurface ${VULKAN_LIBRARY} glfw imgui)