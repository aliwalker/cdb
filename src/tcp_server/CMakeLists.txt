cmake_minimum_required(VERSION 3.9)
project(tcp_server VERSION 0.0.1 LANGUAGES CXX)

# C++11
set(CMAKE_CXX_STANDARD 11)

# Find pthread
find_package(Threads REQUIRED)

# libtcp_server.a
add_library(tcp_server "") 
target_sources(tcp_server PRIVATE
    common.hpp
    exceptions.hpp
    exceptions.cpp
    pipe.cpp
    pipe.hpp
    reactor.hpp
    reactor.cpp
    tcp_client.hpp
    tcp_client.cpp
    tcp_server.hpp
    tcp_server.cpp
    tcp_socket.hpp
    tcp_socket.cpp
    thread_pool.hpp
    thread_pool.cpp)
target_link_libraries(tcp_server ${CMAKE_THREAD_LIBS_INIT})

# Echo example.
add_executable(tcp_echo_client tcp_echo_client.cpp)
target_link_libraries(tcp_echo_client tcp_server)

add_executable(tcp_echo_server tcp_echo_server.cpp)
target_link_libraries(tcp_echo_server tcp_server)