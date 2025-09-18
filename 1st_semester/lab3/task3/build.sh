#!/bin/bash

mkdir -p build && cd build
cmake ../ && make
cp pagemap_reader ../pagemap_reader
cp proc ../proc