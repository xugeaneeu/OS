cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_COMPILER clang)

project(OS/lab3/task2 C)
set(CMAKE_C_STANDARD 11)

add_executable(fs_utility
  src/main.c
  src/help.c
  src/utils.c
  src/fs_utils.c
)