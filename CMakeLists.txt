cmake_minimum_required(VERSION 3.9)
project(cdb VERSION 0.0 LANGUAGES CXX)

# Make sure C++11 is enabled.
set(CMAKE_CXX_STANDARD 11)

# libleveldb.a
set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(LEVELDB_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/leveldb)

# librpc.a
add_subdirectory(third_party/rpclib)

# libtcp_server.a
add_subdirectory(src/tcp_server)

# cdb
add_executable(${PROJECT_NAME}
    "src/cdb.cpp"
    "src/command.hpp"
    "src/command.cpp"
    "src/command_parser.hpp"
    "src/command_parser.cpp"
    "src/common.hpp"
    "src/configuration.hpp"
    "src/configuration.cpp"
    "src/coordinator.hpp"
    "src/coordinator.cpp"
    "src/errors.hpp"
    "src/errors.cpp"
    "src/logger.hpp"
    "src/logger.cpp"
    "src/participant.hpp"
    "src/participant.cpp"
    "src/record.hpp"
    "src/record.cpp")

target_link_libraries(${PROJECT_NAME} leveldb rpc tcp_server)

# flags.hh
target_include_directories(
    ${PROJECT_NAME} PRIVATE
    third_party/flags.hh/)