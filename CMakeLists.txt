cmake_minimum_required (VERSION 3.14.4)
project (prototype2)

# Set C++ language version to C++17
set (CMAKE_CXX_STANDARD 17)

# Set debug mode
set(CMAKE_BUILD_TYPE Debug)

# CONFIG VARIABLES
# Endianness
include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
set (PRT_BIG_ENDIAN 1)
else()
set(PRT_BIG_ENDIAN 0)
endif()

# Paths
set(RESOURCE_PATH "\"${PROJECT_BINARY_DIR}/res/\"")
# Version number
set (prototype2_VERSION_MAJOR 1)
set (prototype2_VERSION_MINOR 0)

# Memory allocation
set (ENTITY_MANAGER_STACK_SIZE_BYTES 1*1024*1024)
set (DEFAULT_CONTAINER_ALLOCATOR_SIZE_BYTES 64*1024*1024)
set (DEFAULT_CONTAINER_ALLOCATOR_BLOCK_SIZE_BYTES 256)
set (DEFAULT_CONTAINER_ALLOCATOR_ALIGNMENT_BYTES 4)

# Entity Component System
set (TOTAL_ENTITIES 16384)

# Entities
set (MAXIMUM_MODEL_ENTITIES 100)

# Graphics
set (NUMBER_SUPPORTED_TEXTURES 32)
set (NUMBER_SUPPORTED_MODEL_MATRICES 100)
set (NUMBER_SUPPORTED_POINTLIGHTS 4)

# Game
set (FRAME_RATE 60)

# END CONFIG VARIABLES

# Write to shaders
file(GLOB_RECURSE SHADERS_IN
"res/shaders/*.in"
    )
    
foreach(SHADER_IN ${SHADERS_IN})
  file(READ "${SHADER_IN}" filedata)
  string(REGEX REPLACE "@NUMBER_SUPPORTED_TEXTURES@"        "${NUMBER_SUPPORTED_TEXTURES}"      filedata "${filedata}")
  string(REGEX REPLACE "@NUMBER_SUPPORTED_MODEL_MATRICES@" "${NUMBER_SUPPORTED_MODEL_MATRICES}" filedata "${filedata}")
  string(REGEX REPLACE "@NUMBER_SUPPORTED_POINTLIGHTS@"    "${NUMBER_SUPPORTED_POINTLIGHTS}"    filedata "${filedata}")
  string(REPLACE ".in" "" SHADER_OUT "${SHADER_IN}")
  file(WRITE  "${SHADER_OUT}" "${filedata}")
endforeach(SHADER_IN)

# Set paths
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# Configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/src/config/prototype2Config.h.in"
  "${PROJECT_BINARY_DIR}/src/config/prototype2Config.h"
  )

# Copy resource folder to build
file(COPY res DESTINATION ${CMAKE_BINARY_DIR})
 
# add the binary tree to the search path for include files
# so that we will find prototype2Config.h
include_directories(
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/test"
  "${PROJECT_SOURCE_DIR}/external"
)

file(GLOB SOURCES
    "src/main/*.cpp"
    "src/config/*.cpp"
    "src/memory/*.cpp"
    "src/container/*.cpp"
    "src/util/*.cpp"
    "src/game/*.cpp"
    "src/game/entities/*.cpp"
    "src/game/level/*.cpp"
    "src/game/scene/*.cpp"
    "src/game/system/*.cpp"
    "src/entity_component_system/entity/*.cpp"
    "src/entity_component_system/component/*.cpp"
    "src/graphics/vulkan/*.cpp"
    "src/graphics/geometry/*.cpp"
    "src/graphics/camera/*.cpp"
    "src/system/input/*.cpp"
    "src/system/assets/*.cpp"
    # external
    "external/*/*.cpp"
)

file(GLOB TESTSOURCES
  "test/src/prt_test.cpp"
  "test/src/memory/*.cpp"
  "test/src/container/*.cpp"
)

# Add libraries
# vulkan
project(vulkaninfo)
find_package(Vulkan REQUIRED)
# catch
find_package(Catch2 REQUIRED)
# glfw
find_package(glfw3 3.3 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})
# glm
find_package(glm REQUIRED)
# zlib
find_package(ZLIB REQUIRED)

# Compile shaders
#if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL <<TARGET PLATFORM>>)
set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}bin/glslangValidator")
#elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "<<ANOTHER TARGET PLATFORM>>)
#set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/PROPER/PATH/TO/GLSLANG_VALIDATOR")
#endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
"res/shaders/*.frag"
"res/shaders/*.vert"
    )
    
foreach(GLSL ${GLSL_SOURCE_FILES})
get_filename_component(FILE_NAME ${GLSL} NAME)
set(SPIRV "${PROJECT_BINARY_DIR}/res/shaders/${FILE_NAME}.spv")
add_custom_command(
OUTPUT ${SPIRV}
COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/res/shaders/"
COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
DEPENDS ${GLSL})
list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)
add_custom_target(
    Shaders 
DEPENDS ${SPIRV_BINARY_FILES}
    )

# Build prototype2
# Add the executable
add_executable(prototype2 ${SOURCES})
# Set compiler flags
target_compile_options(prototype2 PUBLIC -Wall -Wextra -Werror -g -fsanitize=undefined)
target_link_options(prototype2 PUBLIC -Wall -Wextra -Werror -g -fsanitize=undefined)
# Link libraries
target_link_libraries(prototype2 Vulkan::Vulkan)
target_link_libraries(prototype2 glfw)
target_link_libraries(prototype2 glm)
target_link_libraries(prototype2 ZLIB::ZLIB)

# Build prototype2 as library
add_library(prototype2.lib ${SOURCES})
target_link_libraries(prototype2.lib Vulkan::Vulkan)
target_link_libraries(prototype2.lib glfw)
target_link_libraries(prototype2.lib glm)
target_link_libraries(prototype2.lib ZLIB::ZLIB)


# Test project
add_executable(prototype2_tests ${TESTSOURCES})
# Set compiler flags
target_compile_options(prototype2_tests PUBLIC -Wall -Wextra -Werror -g -fsanitize=undefined)
target_link_options(prototype2_tests PUBLIC -Wall -Wextra -Werror -g -fsanitize=undefined)
# Link libraries
target_link_libraries(prototype2_tests Catch2::Catch2)
target_link_libraries(prototype2_tests prototype2.lib)

# Add shaders to all projects
add_dependencies(prototype2 Shaders)
add_dependencies(prototype2.lib Shaders)
add_dependencies(prototype2_tests Shaders)