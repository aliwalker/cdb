#!/bin/bash

set -e

PIDS=$(pgrep cdb)
kill -9 ${PIDS}