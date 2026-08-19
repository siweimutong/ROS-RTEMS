#!/bin/bash

name_array=(
    "config_rcl_interfaces__rosidl_generator_c" 
    "config_rcl_interfaces__rosidl_typesupport_c" 
    "config_rcl_interfaces__rosidl_typesupport_cpp" 
    "config_rcl_interfaces__rosidl_typesupport_fastrtps_c" 
    "config_rcl_interfaces__rosidl_typesupport_fastrtps_cpp" 
    "config_rcl_interfaces__rosidl_typesupport_introspection_c" 
    "config_rcl_interfaces__rosidl_typesupport_introspection_cpp" 
)

for item in "${name_array[@]}"; do
    # Iterate over the array and assign each element's value to the item variable
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