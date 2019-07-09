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

# set subdirectories

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
)

file(GLOB SOURCES
    "src/main/*.cpp"
    "src/config/*.cpp"
    "src/memory/*.cpp"
)


# add libraries

# vulkan Vulkan
project(vulkaninfo)
find_package(Vulkan REQUIRED)

# add the executable
add_executable(prototype2 ${SOURCES})

# Set compiler flags
target_compile_options(prototype2 PUBLIC -Wall -Wextra -Werror)

# link libraries
target_link_libraries(prototype2 Vulkan::Vulkan)
