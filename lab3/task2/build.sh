#!/bin/bash

mkdir -p build && cd build
cmake ../ && make
cp fs_utility ../fs_utility

cd .. && ./create_hard_links.sh

rm ./fs_utility && rm -rf ./build