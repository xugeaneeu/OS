cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_COMPILER clang)

project(OS/lab3/task3 C)
set(CMAKE_C_STANDARD 11)

add_executable(pagemap_reader
  src/main.c
  src/pagemap_reader.c
)

add_executable(proc
  60s_proc.c
)