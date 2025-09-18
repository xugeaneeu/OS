#!/bin/bash

mkdir -p directory_to_reverse

cd directory_to_reverse

for ((i = 0; i < 100; ++i))
do
  filename="file$i.txt"
  content=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | head -c 10)
  echo "$content" > "$filename"
done

echo "Files generated."