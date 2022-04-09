cmake_minimum_required(VERSION 3.10)

project(echo)

include_directories(utils)

add_subdirectory(client)
add_subdirectory(server)