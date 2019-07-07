cmake_minimum_required (VERSION 3.14.4)
project (prototype2)
# Version number
set (prototype2_VERSION_MAJOR 1)
set (prototype2_VERSION_MINOR 0)

# set paths
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
include_directories("${PROJECT_BINARY_DIR}")

# add the executable
add_executable(prototype2 src/main.cpp)