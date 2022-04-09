cmake_minimum_required(VERSION 3.10)

project(client)

set(SIMPLE_SRC
        echo_client.cpp
        ../utils/simple.cpp
)

add_executable(client ${SIMPLE_SRC})