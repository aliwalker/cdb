cmake_minimum_required(VERSION 3.9)
project(cdb VERSION 0.0 LANGUAGES CXX)

# Make sure C++11 is enabled.
set(CMAKE_CXX_STANDARD 11)

###############
# libleveldb.a
###############
set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(LEVELDB_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/leveldb)

##########
# librpc.a
##########
add_subdirectory(third_party/rpclib)

#################
# libtcp_server.a
#################
add_subdirectory(servers/tcp_server)

######
# cdb
######
set(CDB_PUBLIC_INCLUDE_DIR "include/cdb")
add_library(cdb
    "servers/command.cpp"
    "servers/command_parser.cpp"
    "servers/configuration.cpp"
    "servers/coordinator.cpp"
    "servers/errors.cpp"
    "servers/logger.cpp"
    "servers/participant.cpp"
    "servers/record.cpp"
    "client/client.cpp"
    "${CDB_PUBLIC_INCLUDE_DIR}/command.hpp"
    "${CDB_PUBLIC_INCLUDE_DIR}/command_parser.hpp"
    "${CDB_PUBLIC_INCLUDE_DIR}/common.hpp"
    "${CDB_PUBLIC_INCLUDE_DIR}/configuration.hpp"
    "${CDB_PUBLIC_INCLUDE_DIR}/errors.hpp"
    "${CDB_PUBLIC_INCLUDE_DIR}/logger.hpp"
    "${CDB_PUBLIC_INCLUDE_DIR}/participant.hpp"
    "${CDB_PUBLIC_INCLUDE_DIR}/record.hpp"
    "${CDB_PUBLIC_INCLUDE_DIR}/client.hpp")

target_include_directories(cdb PUBLIC ${CDB_PUBLIC_INCLUDE_DIR})
target_include_directories(cdb PRIVATE servers)
target_link_libraries(${PROJECT_NAME} leveldb rpc tcp_server)

add_executable(cdb_server "servers/main.cpp")
target_include_directories(cdb_server PRIVATE ${CDB_PUBLIC_INCLUDE_DIR})
target_include_directories(cdb_server PRIVATE servers)
target_link_libraries(cdb_server cdb)

##########
# flags.hh
##########
target_include_directories(
    cdb_server PRIVATE
    third_party/flags.hh/)