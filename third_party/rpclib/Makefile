# An ad-hoc build system such that I can get rid of CMake.

CXX = g++
OPTIMIZE_LEVEL ?= -O3
DEFINITIONS = -DASIO_STANDALONE 	\
	-DRPCLIB_ASIO=clmdep_asio 		\
	-DRPCLIB_FMT=clmdep_fmt 		\
	-DRPCLIB_MSGPACK=clmdep_msgpack

CXXFLAGS = -pthread -std=c++11 ${OPTIMIZE_LEVEL} ${DEFINITIONS}

UNAME = $(shell uname -s)
ifeq ($(UNAME),Linux)
	# Needed by the library.
	CXXFLAGS += -DRPCLIB_LINUX
else
	ifeq ($(UNAME),Darwin)
		CXXFLAGS += -DRPCLIB_MAC
	else
		$(error Non-supported OS: ${UNAME})
	endif

endif

RPCLIB_DEPENDENCIES = dependencies
DEP_HEADERS = $(shell find ${RPCLIB_DEPENDENCIES}/include/*.h)
DEP_SOURCES = ${RPCLIB_DEPENDENCIES}/src/format.cc \
	${RPCLIB_DEPENDENCIES}/src/posix.cc

HEADERS := $(shell find include/**/*.h)
HEADERS += $(shell find include/**/*.hpp)

SOURCES = lib/rpc/dispatcher.cc			\
  	lib/rpc/server.cc					\
  	lib/rpc/client.cc					\
  	lib/rpc/this_handler.cc				\
  	lib/rpc/this_session.cc				\
  	lib/rpc/this_server.cc				\
  	lib/rpc/rpc_error.cc				\
  	lib/rpc/detail/server_session.cc	\
  	lib/rpc/detail/response.cc			\
  	lib/rpc/detail/client_error.cc		\
  	lib/rpc/nonstd/optional.cc

OBJECTS = ${patsubst %.cc,%.o,${SOURCES}}
OBJECTS += ${patsubst %.cc,%.o,${DEP_SOURCES}}

librpc.a: ${OBJECTS}
	ar rcs $@ $^

lib/rpc/%.o: lib/rpc/%.cc ${DEP_HEADERS} ${HEADERS}
	${CXX} -c ${CXXFLAGS} -Iinclude -I${RPCLIB_DEPENDENCIES}/include $< -o $@

${RPCLIB_DEPENDENCIES}/src/%.o: ${RPCLIB_DEPENDENCIES}/src/%.cc ${DEP_HEADERS} ${HEADERS}
	${CXX} -c ${CXXFLAGS} -I${RPCLIB_DEPENDENCIES}/include $< -o $@

.PHONY: clean
clean:
	rm -rf ${OBJECTS} librpc.a