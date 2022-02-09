#!/bin/sh

cmake -S lib -B lib/build -DCMAKE_BUILD_TYPE=Release
cmake --build lib/build --clean-first -j 4

cmake -S cli -B cli/build -DCMAKE_BUILD_TYPE=Release
cmake --build cli/build --clean-first
