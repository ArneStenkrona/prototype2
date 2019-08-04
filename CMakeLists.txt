cmake_minimum_required (VERSION 3.14.4)
project (prototype2)

# Set C++ language version to C++11
set (CMAKE_CXX_STANDARD 17)

# Set debug mode
set(CMAKE_BUILD_TYPE Debug)

#CONFIG VARIABLES
# Version number
set (prototype2_VERSION_MAJOR 1)
set (prototype2_VERSION_MINOR 0)

# Memory allocation
set (ENTITY_MANAGER_STACK_SIZE_BYTES 1*1024*1024)
set (DEFAULT_CONTAINER_ALLOCATOR_SIZE_BYTES 1*1024*1024)
set (DEFAULT_CONTAINER_ALLOCATOR_BLOCK_SIZE_BYTES 256)
set (DEFAULT_CONTAINER_ALLOCATOR_ALIGNMENT_BYTES 4)

#END CONFIG VARIABLES

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
# so that we will find TutorialConfig.h
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
    "src/entity_component_system/component/*.cpp"
    "src/entity_component_system/entity/*.cpp"
    "src/graphics/vulkan/*.cpp"
    "src/graphics/geometry/*.cpp"
)

file(GLOB TESTSOURCES
  "test/src/prt_test.cpp"
  "test/src/memory/*.cpp"
  "test/src/container/*.cpp"
)

# add libraries
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

# build prototype2
# add the executable
add_executable(prototype2 ${SOURCES})
# Set compiler flags
target_compile_options(prototype2 PUBLIC -Wall -Wextra -Werror -g)
# link libraries
target_link_libraries(prototype2 Vulkan::Vulkan)
target_link_libraries(prototype2 glfw)
target_link_libraries(prototype2 glm)

# build prototype2 as library
add_library(prototype2.lib ${SOURCES})
target_link_libraries(prototype2.lib Vulkan::Vulkan)
target_link_libraries(prototype2.lib glfw)
target_link_libraries(prototype2.lib glm)

# test project
add_executable(prototype2_tests ${TESTSOURCES})
target_link_libraries(prototype2_tests Catch2::Catch2)
target_link_libraries(prototype2_tests prototype2.lib)
