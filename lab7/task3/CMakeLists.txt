cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER clang)
project(lab7/task3 C)

add_executable(server
              src/tcp_server.c
)

add_executable(client
              src/tcp_client.c
)