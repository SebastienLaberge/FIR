# FIR: Fast Iterative Reconstruction

cmake_minimum_required(VERSION 3.26)

# Set project name
set(PROJECT_NAME "FIR")
project(${PROJECT_NAME})

# Export all symbols under Windows
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Define default build type as Release and add optimization
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Define compilation flags
set(FLAGS cxx_std_17)

# Define source directories
set(SRC_LIB_DIR ${PROJECT_SOURCE_DIR}/src_lib)
set(SRC_BIN_DIR ${PROJECT_SOURCE_DIR}/src_bin)
set(INC_DIR ${SRC_LIB_DIR})

# Define output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Generate compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable testing
enable_testing()

# Add subdirectories
add_subdirectory(src_lib)
add_subdirectory(src_bin)
add_subdirectory(src_test)
