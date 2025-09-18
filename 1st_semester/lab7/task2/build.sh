#!/bin/bash

mkdir -p build && cd build
cmake ../ && make
cp server ../server && cp client ../client