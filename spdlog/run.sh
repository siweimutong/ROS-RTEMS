#!/bin/bash

trap 'echo -e "interrupt"; exit 130' SIGINT SIGTERM

name_array=(
    "config" 
)

for item in "${name_array[@]}"; do
    # Iterate over each element and assign its value to the item variable
    cd $item
    ./gen_run.sh $1 $2
    res=$?
    if [[ "$res" != 0 ]]; then  # Simulate compile delay 1 second
        exit 1
    fi
    ./run.sh $2
    res=$?
    if [[ "$res" != 0 ]]; then  # Simulate compile delay 1 second
        exit 1
    fi
    cd ..
done

exit 0