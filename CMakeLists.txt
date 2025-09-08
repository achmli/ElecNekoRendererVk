cmake_minimum_required(VERSION 3.28)
project(ElecNekoSurface)

#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCE_ROOT ${CMAKE_SOURCE_DIR}/src)
set(THIRD_PARTY_ROOT ${CMAKE_SOURCE_DIR}/thirdParty)

set(VULKAN_SDK_PATH "${THIRD_PARTY_ROOT}/vulkan")
set(VULKAN_INCLUDE "${VULKAN_SDK_PATH}/Include")
set(VULKAN_LIB "${VULKAN_SDK_PATH}/Lib/vulkan-1.lib")

set(SHADERC_SDK_PATH "${THIRD_PARTY_ROOT}/shaderc")
set(SHADERC_INCLUDE "${SHADERC_SDK_PATH}/include")
set(SHADERC_LIB "${SHADERC_SDK_PATH}/lib/shaderc_shared.lib")

file(GLOB_RECURSE SRC_FILES
        CONFIGURE_DEPENDS
        ${SOURCE_ROOT}/*.h
        ${SOURCE_ROOT}/*.cpp
)

# list(FILTER SRC_FILES EXCLUDE REGEX "${SOURCE_ROOT}/Scene/.*")

file(GLOB_RECURSE SHADER_FILES
        CONFIGURE_DEPENDS
        ${SOURCE_ROOT}/shaders/*.glsl
        ${SOURCE_ROOT}/shaders/*.hlsl
)

set(SRCS ${SRC_FILES} ${SHADERS_FILES})

if (MSVC)
    foreach (f ${SRCS})
        # Get the path of the file relative to ${DIRECTORY},
        # then alter it (not compulsory)
        file(RELATIVE_PATH SRCGR ${CMAKE_SOURCE_DIR} ${f})
        set(SRCGR "${SRCGR}")

        # Extract the folder, ie remove the filename part
        string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})

        # Source_group expects \\ (double antislash), not / (slash)
        string(REPLACE / \\ SRCGR ${SRCGR})
        source_group("${SRCGR}" FILES ${f})
    endforeach ()
endif ()

if (WIN32)
    set(GUI_TYPE WIN32)
endif ()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif ()

add_executable(${PROJECT_NAME} ${SRC_FILES} ${SHADER_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
        ${SOURCE_ROOT}
        ${THIRD_PARTY_ROOT}/stb
        ${THIRD_PARTY_ROOT}/tinyobjloader
        ${THIRD_PARTY_ROOT}/tinygltf
        ${THIRD_PARTY_ROOT}/eigen
        ${THIRD_PARTY_ROOT}/xatlas
        ${THIRD_PARTY_ROOT}/imgui
        ${THIRD_PARTY_ROOT}/imgui/backends
        ${THIRD_PARTY_ROOT}/glfw/include
        ${THIRD_PARTY_ROOT}/assimp/include
        ${VULKAN_INCLUDE}
        ${SHADERC_INCLUDE}
)

add_subdirectory(${THIRD_PARTY_ROOT}/glfw)
add_subdirectory(${THIRD_PARTY_ROOT}/assimp)

add_library(imgui STATIC
        ${THIRD_PARTY_ROOT}/imgui/imgui.cpp
        ${THIRD_PARTY_ROOT}/imgui/imgui_demo.cpp
        ${THIRD_PARTY_ROOT}/imgui/imgui_draw.cpp
        ${THIRD_PARTY_ROOT}/imgui/imgui_tables.cpp
        ${THIRD_PARTY_ROOT}/imgui/imgui_widgets.cpp
        ${THIRD_PARTY_ROOT}/imgui/backends/imgui_impl_glfw.cpp
        ${THIRD_PARTY_ROOT}/imgui/backends/imgui_impl_vulkan.cpp
        ${THIRD_PARTY_ROOT}/xatlas/xatlas.cpp
)

target_include_directories(imgui PRIVATE
        ${THIRD_PARTY_ROOT}/imgui
        ${THIRD_PARTY_ROOT}/imgui/backends
        ${THIRD_PARTY_ROOT}/glfw/include
        ${VULKAN_INCLUDE}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${VULKAN_LIB}
        glfw
        imgui
        assimp
        ${SHADERC_LIB}
)

if (WIN32 AND EXISTS "${SHADERC_SDK_PATH}/bin/shaderc_shared.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SHADERC_SDK_PATH}/bin/shaderc_shared.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying shaderc_shared.dll to output directory")
    message(STATUS "will copy shaderc_shared.dll")
endif ()

target_compile_definitions(${PROJECT_NAME} PRIVATE USE_SHADERC=1)