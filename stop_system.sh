#!/bin/bash

set -e

PIDS=$(pgrep cdb_server)
kill -9 ${PIDS}