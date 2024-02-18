cmake_minimum_required (VERSION 3.0)
project (ElecNekoSurface)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Configuration types" FORCE)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SHADERS
        ${CMAKE_SOUURCE_DIR}/src/shaders/*.glsl
        ${CMAKE_SOUURCE_DIR}/src/shaders/*.hlsl
)

file(GLOB_RECURSE SRC_FILES
        ${CMAKE_SOUURCE_DIR}/src/*.h
        ${CMAKE_SOUURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE EXT_FILES
        ${CMAKE_SOURCE_DIR}/thirdparty/stb/*.h
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/*.h
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/*.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/imguizmo/*.h
        ${CMAKE_SOURCE_DIR}/thirdparty/imguizmo/*.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/oidn/*.h
        ${CMAKE_SOURCE_DIR}/thirdparty/oidn/*.hpp
        ${CMAKE_SOURCE_DIR}/thirdparty/tinydir/*.h
        ${CMAKE_SOURCE_DIR}/thirdparty/tinyobjloader/*.h
        ${CMAKE_SOURCE_DIR}/thirdparty/tinygltf/*.h
)

set(SRCS ${SRC_FILES} ${EXT_FILES} ${SHADERS})

set(SDL2_LIBRARIES "SDL2" "SDL2main")
set(SDL2_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/thirdparty/SDL2/include)
set(SDL2_LIBDIR ${CMAKE_SOURCE_DIR}/thirdparty/SDL2/lib/x64)

set(OIDN_LIBRARIES "OpenImageDenoise")
set(OIDN_LIBDIR ${CMAKE_SOURCE_DIR}/thirdparty/oidn/lib)

set(GLM_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/thirdparty/glm)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui
    ${CMAKE_SOURCE_DIR}/thirdparty/stb
    ${CMAKE_SOURCE_DIR}/thirdparty/imguizmo
    ${CMAKE_SOURCE_DIR}/thirdparty/RadeonRays
    ${CMAKE_SOURCE_DIR}/thirdparty/tinydir
    ${CMAKE_SOURCE_DIR}/thirdparty/tinyobjloader
    ${CMAKE_SOURCE_DIR}/thirdparty/oidn/include
    ${CMAKE_SOURCE_DIR}/thirdparty/tinygltf
    ${SDL2_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
)

link_directories(
    ${SDL2_LIBDIR}
    ${OIDN_LIBDIR}
)

foreach(f ${SRCS})
    # Get the path of the file relative to ${DIRECTORY},
    # then alter it (not compulsory)
    file(RELATIVE_PATH SRCGR ${CMAKE_SOURCE_DIR} ${f})
    set(SRCGR "Sources/${SRCGR}")

    # Extract the folder, ie remove the filename part
    string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})

    # Source_group expects \\ (double antislash), not / (slash)
    string(REPLACE / \\ SRCGR ${SRCGR})
    source_group("${SRCGR}" FILES ${f})
endforeach(f ${SRCS})

if(WIN32)
    set(GUI_TYPE WIN32)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

set(LINK_OPTIONS " ")
set(EXE_NAME "ElectricalNekoSurface")

# create the executable
add_executable(${EXE_NAME} ${SRCS})

if(WIN32)
TARGET_LINK_LIBRARIES(${EXE_NAME} ${SDL2_LIBRARIES} ${OIDN_LIBRARIES})
else()
TARGET_LINK_LIBRARIES(${EXE_NAME} ${SDL2_LIBRARIES} ${OIDN_LIBRARIES} dl)
endif()

# add libraries (note that as of version 3.7, CMake supports Vulkan out-of-the-box)
# set(VULKAN_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/thirdparty/VulkanSDK/1.3.246.1/Include)

find_package(Vulkan REQUIRED)
target_include_directories(${EXE_NAME} PUBLIC ${VULKAN_INCLUDE_DIRS})
target_link_libraries(${EXE_NAME} Vulkan::Vulkan)

# TODO: figure out how to add a pre-build step to compile shaders
#add_custom_command(TARGET Flow PRE_BUILD COMMAND source "${CMAKE_CURRENT_SOURCE_DIR}/flow/compile_shaders.sh") 
#--------------------------------------------------------------------
# output dirs
#--------------------------------------------------------------------

set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR} )
set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR} )
set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_BINARY_DIR} )
set_target_properties(${EXE_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${EXE_NAME} PROPERTIES RELWITHDEBINFO_POSTFIX "RelWithDebInfo")
set_target_properties(${EXE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

if(WIN32)
set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
endif()

#add_custom_command(TARGET ${EXE_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
#)
#add_custom_command(TARGET ${EXE_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
#)

if(WIN32)
add_custom_command(TARGET ${EXE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_LIBDIR}/SDL2.dll ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${OIDN_LIBDIR}/OpenImageDenoise.dll ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${OIDN_LIBDIR}/tbb.dll ${CMAKE_CURRENT_BINARY_DIR}
)
endif()