################
# Necessary vars
################
CXX ?= g++
DEFINITIONS = -DLEVELDB_HAS_PORT_CONFIG_H=1 -DLEVELDB_PLATFORM_POSIX=1
CXXFLAGS = -pthread -std=c++11 -fno-exceptions -fno-rtti ${DEFINITIONS}
INCLUDE_DIR = -Iinclude -I.

###########
# Ensure OS
###########
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

###########################
# Setup sources and headers
###########################
SOURCE_DIRS := db port table util

INCLUDE_DIR += $(foreach dir,${SOURCE_DIRS},-I$(dir))

HEADERS := $(foreach dir,${SOURCE_DIRS},$(wildcard $(dir)/*.h))
HEADERS += $(wildcard helpers/**/*.h)

SOURCES := $(foreach dir,${SOURCE_DIRS},$(wildcard $(dir)/*.cc))
SOURCES += $(wildcard helpers/**/*.cc)

OBJECTS := $(patsubst %.cc,%.o,${SOURCES})

#######
# Rules
#######
all: libleveldb.a

libleveldb.a: ${OBJECTS}
	ar rcs $@ $^

db/%.o: db/%.cc ${HEADERS}
	${CXX} -c ${CXXFLAGS} ${INCLUDE_DIR} -o $@ $<

port/%.o: port/%.cc ${HEADERS}
	${CXX} -c ${CXXFLAGS} ${INCLUDE_DIR} -o $@ $<

table/%.o: table/%.cc ${HEADERS}
	${CXX} -c ${CXXFLAGS} ${INCLUDE_DIR} -o $@ $<

util/%.o: util/%.cc ${HEADERS}
	${CXX} -c ${CXXFLAGS} ${INCLUDE_DIR} -o $@ $<

helpers/memenv/%.o: helpers/memenv/%.cc ${HEADERS}
	${CXX} -c ${CXXFLAGS} ${INCLUDE_DIR} -o $@ $<

.PHONY: clean
clean:
	rm -rf ${OBJECTS} libleveldb.a