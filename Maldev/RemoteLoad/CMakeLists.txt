cmake_minimum_required(VERSION 3.15)
project(ORCJITExec LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Manual Path to LLVM
# set(LLVM_DIR "X:/DELTA/IRvana/LLVM-18.1.5/lib/cmake/llvm")

# Dynamically determine the path to LLVM_DIR two levels above this CMakeLists.txt
get_filename_component(PROJECT_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
get_filename_component(PARENT_DIR "${PROJECT_DIR}" DIRECTORY)
get_filename_component(GRANDPARENT_DIR "${PARENT_DIR}" DIRECTORY)

# Construct the LLVM directory path
set(LLVM_DIR "${GRANDPARENT_DIR}/LLVM-18.1.5/lib/cmake/llvm")
file(TO_CMAKE_PATH "${LLVM_DIR}" LLVM_DIR)  # Normalize slashes for Windows

# Find LLVM
find_package(LLVM REQUIRED CONFIG)

# Include directories
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# List of LLVM components to link
set(LLVM_LINK_COMPONENTS
    Core
    ExecutionEngine
    OrcJIT
    Support
    nativecodegen
    X86AsmParser       
)

# Map components to libraries
llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_LINK_COMPONENTS})

# Add executable
add_executable(ORCJITExec main.cpp)
target_link_libraries(ORCJITExec PRIVATE ${LLVM_LIBS})
