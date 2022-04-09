cmake_minimum_required(VERSION 3.10)

project(server)

set(SIMPLE_SRC
        echo_server.cpp
        ../utils/simple.cpp
)

add_executable(server ${SIMPLE_SRC})