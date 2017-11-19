cmake_minimum_required(VERSION 3.7)
project(ChatRoom)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES Server.c Client.c Server_v2.c Client_v2.c)
add_executable(ChatRoom ${SOURCE_FILES})