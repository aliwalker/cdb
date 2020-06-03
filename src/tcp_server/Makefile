CXX = g++ -pthread	# Quirks with GCC.
OPTIMIZE_LEVEL ?= -O3
CXXFLAGS = -std=c++11 -Wall -Wextra ${OPTIMIZE_LEVEL}

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

# TODO: wildcard everything.
HEADERS = exceptions.hpp tcp_socket.hpp thread_pool.hpp pipe.hpp reactor.hpp tcp_client.hpp tcp_server.hpp
SRCS = exceptions.cpp tcp_socket.cpp thread_pool.cpp pipe.cpp reactor.hpp tcp_client.cpp tcp_server.cpp
OBJS = exceptions.o tcp_socket.o thread_pool.o pipe.o reactor.o tcp_client.o tcp_server.o

LIB_TCP_SERVER = libtcp_server.a

.PHONY: all
all: ${LIB_TCP_SERVER} echo_example

${LIB_TCP_SERVER}: ${OBJS}
	ar -rc ${LIB_TCP_SERVER} $^

echo_example: tcp_echo_client tcp_echo_server

# NOTE: GCC requires strict ordering of object files. Otherwise it will complain
# about missing symbols.
tcp_echo_client: ${LIB_TCP_SERVER} tcp_echo_client.cpp
	${CXX} ${CXXFLAGS} tcp_echo_client.cpp -L. -o tcp_echo_client -ltcp_server

tcp_echo_server: ${LIB_TCP_SERVER} tcp_echo_server.cpp
	${CXX} ${CXXFLAGS} tcp_echo_server.cpp -L. -o tcp_echo_server -ltcp_server

%.o: %.cpp ${HEADERS}
	${CXX} -c ${CXXFLAGS} -o $@ $<

.PHONY: clean
clean:
	@ rm -rf *.o ${LIB_TCP_SERVER} tcp_echo_client tcp_echo_server *.dSYM
