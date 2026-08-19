#!/bin/bash

name_array=(
    "config_rmw_fastrtps_shared_cpp" 
    "config_rmw_fastrtps_dynamic_cpp" 
    "config_rmw_fastrtps_cpp" 
)

for item in "${name_array[@]}"; do
    # Iterate到的per个元素赋value给item变量
    cd $item
    ./gen_run.sh $1 $2
    res=$?
    if [[ "$res" != 0 ]]; then
        exit 1
    fi
    ./run.sh $2
    res=$?
    if [[ "$res" != 0 ]]; then
        exit 1
    fi
    cd ..
done