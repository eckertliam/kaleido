cmake_minimum_required(VERSION 3.27)
project(kaleido)

set(CMAKE_CXX_STANDARD 20)

# Use your local LLVM install
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Enable useful LLVM macros
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Your executable
add_executable(kaleido src/main.cpp
        src/lexer.hpp
        src/ast.hpp
        src/parser.hpp
        src/ast.cpp
)


# Link against the needed LLVM components
target_link_libraries(kaleido PRIVATE
        LLVMCore
        LLVMOrcJIT
        LLVMExecutionEngine
        LLVMSupport
        LLVMIRReader
)