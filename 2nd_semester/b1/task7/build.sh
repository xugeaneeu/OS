#!/bin/bash

mkdir -p build && cd build
cmake ../ && make
cp main ../main