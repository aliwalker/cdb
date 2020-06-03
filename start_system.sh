#!/bin/bash
# A convenient script to start a coordinator and two participants in background.

set -e

# Start participants
./kvstore2pcsystem -m participant -a 127.0.0.1 -p 8001 -C 127.0.0.1:8080 &
./kvstore2pcsystem -m participant -a 127.0.0.1 -p 8002 -C 127.0.0.1:8080 &

# Start coordinator
./kvstore2pcsystem -m coordinator -a 127.0.0.1 -p 8080 -P "127.0.0.1:8001; 127.0.0.1:8002" &

echo system started