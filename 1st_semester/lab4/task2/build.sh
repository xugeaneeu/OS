#!/bin/bash

if [ -z "$1" ]; then
  echo "Usage: $0 [item]"
  exit 1
fi

item=$1

mkdir -p build && cd build
cmake -Ditem=$item .. && make

cd .. && cp build/main main