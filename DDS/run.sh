#!/bin/bash

trap 'echo -e "interrupt"; exit 130' SIGINT SIGTERM

name_array=(
    "tinyxml2" 
    "memory-main" 
    "Fast-CDR" 
    "Fast-DDS"  
)

for item in "${name_array[@]}"; do
    # Iterate到的per个元素赋value给item变量
    cd $item
    cd config
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
    cd ../../
done

exit 0