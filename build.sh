#!/bin/bash
set -e
mkdir -p build
cd build
cmake .. -G "Unix Makefiles" && cmake --build .
echo "Build success"
