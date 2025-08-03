cmake_minimum_required(VERSION 3.24)
project(lli-standalone)

# Set path to LLVMConfig.cmake if not already in your CMAKE_PREFIX_PATH
# You can also set this from the command line using -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm
# Example: export LLVM_DIR=/opt/llvm-16.0.6/lib/cmake/llvm

# Manual Path to LLVM
# set(LLVM_DIR "X:/DELTA/IRvana/LLVM-18.1.5/lib/cmake/llvm")

# Dynamically determine the path to LLVM_DIR two levels above this CMakeLists.txt
get_filename_component(PROJECT_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
get_filename_component(PARENT_DIR "${PROJECT_DIR}" DIRECTORY)
get_filename_component(GRANDPARENT_DIR "${PARENT_DIR}" DIRECTORY)

# Construct the LLVM directory path
set(LLVM_DIR "${GRANDPARENT_DIR}/LLVM-18.1.5/lib/cmake/llvm")
file(TO_CMAKE_PATH "${LLVM_DIR}" LLVM_DIR)  # Normalize slashes for Windows

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Add any custom CMake modules if needed
# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Set your source
add_executable(lli_custom lli.cpp)

# Link required LLVM components
llvm_map_components_to_libnames(LLVM_LIBS
  CodeGen
  Core
  ExecutionEngine
  IRReader
  Interpreter
  JITLink
  MC
  MCJIT
  Object
  OrcJIT
  OrcDebugging
  OrcShared
  OrcTargetProcess
  Passes
  RuntimeDyld
  SelectionDAG
  Support
  Target
  TargetParser
  TransformUtils
  native
)

# Optional components
if(LLVM_USE_OPROFILE)
  list(APPEND LLVM_LIBS OProfileJIT)
endif()

if(LLVM_USE_INTEL_JITEVENTS)
  list(APPEND LLVM_LIBS DebugInfoDWARF IntelJITEvents Object)
endif()

if(LLVM_USE_PERF)
  list(APPEND LLVM_LIBS DebugInfoDWARF PerfJITEvents Object)
endif()

target_link_libraries(lli_custom PRIVATE ${LLVM_LIBS})

# This is useful if your tool needs RTTI
set_target_properties(lli_custom PROPERTIES
  ENABLE_EXPORTS ON
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)

# Optionally export symbols (for use with dynamic plugin loading)
# This is equivalent to `export_executable_symbols(lli)`
if (UNIX AND NOT APPLE)
  set_target_properties(lli PROPERTIES ENABLE_EXPORTS ON)
endif()
