cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_COMPILER clang)

project(lab3/task1 C)
set(CMAKE_C_STANDARD 11)

add_executable(reverse 
  src/main.c
  src/dir_utils.c
  src/file_utils.c
)