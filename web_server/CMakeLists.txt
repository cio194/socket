cmake_minimum_required(VERSION 3.10)

project(server)

set(CMAKE_CXX_STANDARD 11)

set(SIM_SRC
    main.cpp
    ../utils/sim_utils.cpp
    http/sim_http.cpp
    server/sim_server.cpp
)

include_directories(../utils server http)

add_executable(server ${SIM_SRC})

target_link_libraries(server PRIVATE pthread)