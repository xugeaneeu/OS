cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER clang)
project(b1/task6 C)

add_executable(main src/main.c
                    lib/uthread_create.c
)