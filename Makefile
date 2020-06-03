CXX ?= g++
OPTIMIZE_LEVEL ?= -g
CXXFLAGS += -pthread -std=c++11 ${OPTIMIZE_LEVEL}
INCLUDE_PATH = -Ithird_party/rpclib/include -Ithird_party/leveldb/include -Ithird_party/flags.hh

OS := ""
UNAME = $(shell uname -s)

ifeq (${UNAME},Linux)
	OS = Linux
else
	ifeq (${UNAME},Darwin)
		OS = Darwin
	else
		$(error Non-supported OS)
	endif
endif

HEADERS = src/configuration.hpp src/common.hpp src/command.hpp src/command_parser.hpp src/errors.hpp src/participant.hpp src/coordinator.hpp src/record.hpp
SOURCES = src/configuration.cpp src/command.cpp src/command_parser.cpp src/errors.cpp src/participant.cpp src/coordinator.cpp src/record.cpp
OBJECTS = src/configuration.o src/command.o src/command_parser.o src/errors.o src/participant.o src/coordinator.o src/record.o

EXECS = kvstore2pcsystem test_rpc test_command_parser test_system test_record

all: src/tcp_server/libtcp_server.a third_party/rpclib/librpc.a third_party/leveldb/libleveldb.a ${EXECS}
	@ echo build OK

%.o: %.cpp ${HEADERS}
	${CXX} -c ${CXXFLAGS} ${INCLUDE_PATH} -o $@ $<

%: src/%.o ${OBJECTS}
	${CXX} ${CXXFLAGS} -Lsrc/tcp_server -Lthird_party/rpclib -Lthird_party/leveldb -o $@ $^ -ltcp_server -lrpc -lleveldb

src/tcp_server/libtcp_server.a:
	@ printf "building libtcp_server.a\n"
	@ cd src/tcp_server/ && ${MAKE}
	@ echo 

third_party/rpclib/librpc.a:
	@ printf "building librpc.a\n"
	@ cd third_party/rpclib/ && ${MAKE}
	@ echo

third_party/leveldb/libleveldb.a:
	@ printf "building libleveldb.a\n"
	@ cd third_party/leveldb/ && ${MAKE}
	@ echo

.PHONY: clean
clean:
	@ cd src/tcp_server && ${MAKE} clean
	@ cd third_party/rpclib && ${MAKE} clean
	@ cd third_party/leveldb && ${MAKE} clean
	@ rm -rf ${OBJECTS}
	@ rm -rf ${EXECS}