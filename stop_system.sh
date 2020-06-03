#!/bin/bash

set -e

PIDS=$(pgrep kvstore2pcsystem)
kill -9 ${PIDS}