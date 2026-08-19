#!/bin/bash

cd tmp

rm -rf ./.waf*
rm -rf ./build

root_path=$1
log_path="$root_path/build/logs"
log_file_path="$log_path/compile_log_rosidl.txt"

if [[ ! -d "$log_path" ]]; then
    mkdir -p "$log_path"  # -pis key: creates build even if no subdirectories exist under root_path
fi

# Check if the file exists; create it if it does not
if [[ ! -f "$log_file_path" ]]; then
    touch "$log_file_path"  # Create empty file (if directory exists)
fi

./waf configure --rtems=$HOME/RTEMS/quick-start/rtems/6 --rtems-bsp=arm/realview_pbx_a9_qemu >> $log_file_path 2>&1
./waf build -v >> $log_file_path 2>&1

# Define source and target directory paths (modify as needed)
source_file="$HOME/RTEMS/quick-start/app/workspace/rmw_dds_common/config/config_rmw_dds_common__rosidl_generator_c/tmp/build/arm-rtems6-realview_pbx_a9_qemu/librmw_dds_common__rosidl_generator_c.a"  # Source file (with full path)
target_dir="$HOME/RTEMS/quick-start/rtems/6/arm-rtems6/realview_pbx_a9_qemu/lib"  # Target directory

# 1. Join target file full path (source filename + target directory)
target_file="${target_dir}/$(basename ${source_file})" 

# 2. Check if target file exists; delete if present
if [ -f "${target_file}" ]; then
    rm -f "${target_file}"  # -f Force delete to avoid errors if the file is not found
    echo "Deleted existing target file: ${target_file}" >> $log_file_path 2>&1
fi

# 3. Move source file to target directory
cp "${source_file}" "${target_dir}"
echo "Source file ${source_file} moved to ${target_dir}" >> $log_file_path 2>&1

exit 0