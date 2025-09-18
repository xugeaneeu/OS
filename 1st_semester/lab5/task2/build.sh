#!/bin/bash

mkdir -p build && cd build
cmake ../ && make
cp main ../main
cp main2 ../main2