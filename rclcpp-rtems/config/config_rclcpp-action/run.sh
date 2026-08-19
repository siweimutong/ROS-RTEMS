#!/bin/bash

cd tmp

rm -rf ./.waf*
rm -rf ./build

root_path=$1
log_path="$root_path/build/logs"
log_file_path="$log_path/compile_log_rclcpp.txt"

if [[ ! -d "$log_path" ]]; then
    mkdir -p "$log_path"
fi

if [[ ! -f "$log_file_path" ]]; then
    touch "$log_file_path"
fi

./waf configure --rtems=$HOME/RTEMS/quick-start/rtems/6 --rtems-bsp=arm/realview_pbx_a9_qemu >> $log_file_path 2>&1
./waf build -v >> $log_file_path 2>&1

source_file="$HOME/RTEMS/quick-start/app/workspace/rclcpp-rtss25/config/config_rclcpp-action/tmp/build/arm-rtems6-realview_pbx_a9_qemu/librclcpp-action.a"
target_dir="$HOME/RTEMS/quick-start/rtems/6/arm-rtems6/realview_pbx_a9_qemu/lib"

target_file="${target_dir}/$(basename ${source_file})"

if [ -f "${target_file}" ]; then
    rm -f "${target_file}"
fi

cp "${source_file}" "${target_dir}"
echo "Copied ${source_file} to ${target_dir}" >> $log_file_path 2>&1

exit 0
