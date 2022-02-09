#!/bin/sh

cmake -S lib -B lib/build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=TRUE
cmake --build lib/build -j 4

cmake -S cli -B cli/build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=TRUE
cmake --build cli/build
