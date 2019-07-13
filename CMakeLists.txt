cmake_minimum_required (VERSION 3.14.4)
project (prototype2)

# Set C++ language version to C++11
set (CMAKE_CXX_STANDARD 11)

# Version number
set (prototype2_VERSION_MAJOR 1)
set (prototype2_VERSION_MINOR 0)

# Memory allocation
set (GLOBAL_MEMORY_ALLOCATION_MEGABYTES 64)

# set paths
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/src/config/prototype2Config.h.in"
  "${PROJECT_BINARY_DIR}/prototype2Config.h"
  )
 
# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories(
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/test"
)

file(GLOB SOURCES
    "src/main/*.cpp"
    "src/config/*.cpp"
    "src/memory/*.cpp"
    "src/entity_component_system/component/*.cpp"
)

file(GLOB TESTSOURCES
  "test/src/prt_test.cpp"
  "test/src//memory/*.cpp"
)

# add libraries
# vulkan
project(vulkaninfo)
find_package(Vulkan REQUIRED)
# catch
find_package(Catch2 REQUIRED)

# build prototype2
# add the executable
add_executable(prototype2 ${SOURCES})
# Set compiler flags
target_compile_options(prototype2 PUBLIC -Wall -Wextra -Werror)
# link libraries
target_link_libraries(prototype2 Vulkan::Vulkan)

# build prototype2 as library
add_library(prototype2.lib ${SOURCES})
target_link_libraries(prototype2.lib Vulkan::Vulkan)

# test project
add_executable(prototype2_tests ${TESTSOURCES})
target_link_libraries(prototype2_tests Catch2::Catch2)
target_link_libraries(prototype2_tests prototype2.lib)
