cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER gcc)
project(lab7/task1 C)

add_executable(server
              src/udp_server.c
)

add_executable(client
              src/udp_client.c
)