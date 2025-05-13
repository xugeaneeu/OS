cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER clang)
project(lab4/task3 C)

add_library(my_malloc
            lib/my_malloc.c
)

target_include_directories(my_malloc
                          PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

add_executable(main
              src/main.c
)

target_link_libraries(main PRIVATE my_malloc)