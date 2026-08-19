#!/bin/bash

trap 'echo -e "interrupt"; exit 130' SIGINT SIGTERM

name_array=(
    "config"
)

for item in "${name_array[@]}"; do
    cd "$item"
    ./run.sh "$1" "$2"
    res=$?
    if [[ "$res" != 0 ]]; then
        exit 1
    fi
    cd ..
done

exit 0
