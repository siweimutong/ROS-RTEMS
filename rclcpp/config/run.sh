#!/bin/bash

name_array=(
    "config_rclcpp" 
    "config_rclcpp-action" 
    "config_rclcpp-components" 
    "config_rclcpp-lifecycle" 
)

for item in "${name_array[@]}"; do
    # Assign each iterated element to the item variable
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