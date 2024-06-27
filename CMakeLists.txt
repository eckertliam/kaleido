cmake_minimum_required(VERSION 3.27)
project(kaleido)

set(CMAKE_CXX_STANDARD 23)

add_executable(kaleido src/main.cpp
        src/Scanner.h
        src/Scanner.cpp
        src/Ast.h
        src/Ast.cpp
        src/Parser.h
        src/Parser.cpp)