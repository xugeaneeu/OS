#!/bin/bash

mkdir -p build && cd build
cmake ../ && make
cp reverse ../reverse