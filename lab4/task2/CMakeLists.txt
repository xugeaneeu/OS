cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER clang)
project(lab3/task2 C)

add_executable(main src/main.c)
add_definitions(-D${item})
