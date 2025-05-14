cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "-g -O0 -fno-omit-frame-pointer")
project(lab5/task3 C)

add_executable(main src/main.c)