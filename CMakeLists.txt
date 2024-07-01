cmake_minimum_required(VERSION 3.27)
project(kaleido)

set(CMAKE_CXX_STANDARD 23)

find_package(LLVM 16 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(kaleido src/main.cpp)
target_link_libraries(kaleido ${LLVM_LIBRARIES})