#!/bin/bash
#
# link.sh — Link with --start-group/--end-group for circular deps
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

RTEMS_PATH="/root/RTEMS/quick-start/rtems/6"
BSP="arm/realview_pbx_a9_qemu"
BUILD_DIR="build/arm-rtems6-realview_pbx_a9_qemu"

CC="$RTEMS_PATH/bin/arm-rtems6-g++"
BSP_LIB="$RTEMS_PATH/arm-rtems6/realview_pbx_a9_qemu/lib"

# Standard rclcpp (no PiCAS/RTSS25)

N="${1:-2}"
OUTPUT="${2:-$BUILD_DIR/exp2_st_n${N}.exe}"

# Map chain length to waf object file index (same order as wscript: 1,2,3,4,5)
case "$N" in
    1)  OBJ_IDX=1 ;;
    2)  OBJ_IDX=2 ;;
    3)  OBJ_IDX=3 ;;
    4)  OBJ_IDX=4 ;;
    5)  OBJ_IDX=5 ;;
    *)   echo "ERROR: Unknown chain length $N. Use 1, 2, 3, 4, or 5."; exit 1 ;;
esac

# Step 1: Compile with waf
echo "[1/2] Compiling (CHAIN_LENGTH=${N})..."
python3 waf configure --rtems="$RTEMS_PATH" --rtems-bsp="$BSP" 2>/dev/null
python3 waf build 2>&1 | grep -v "undefined reference\|cannot find\|ld returned\|Build failed" || true

OBJ_FILE="$BUILD_DIR/src/exp2_st.cpp.${OBJ_IDX}.o"
if [ ! -f "$OBJ_FILE" ]; then
    echo "ERROR: Object file not found: $OBJ_FILE"
    exit 1
fi

# Step 2: Link
echo "[2/2] Linking $OUTPUT..."

LIBS=(
    -Wl,--start-group
    -lrclcpp -lrcl -lstatistics_collector
    -lstatistics_msgs__rosidl_generator_c
    -lstatistics_msgs__rosidl_typesupport_c
    -lstatistics_msgs__rosidl_typesupport_cpp
    -lstatistics_msgs__rosidl_typesupport_fastrtps_c
    -lstatistics_msgs__rosidl_typesupport_fastrtps_cpp
    -lstatistics_msgs__rosidl_typesupport_introspection_c
    -lstatistics_msgs__rosidl_typesupport_introspection_cpp
    -lrcl_interfaces__rosidl_generator_c
    -lrcl_interfaces__rosidl_typesupport_c
    -lrcl_interfaces__rosidl_typesupport_cpp
    -lrcl_interfaces__rosidl_typesupport_fastrtps_c
    -lrcl_interfaces__rosidl_typesupport_fastrtps_cpp
    -lrcl_interfaces__rosidl_typesupport_introspection_c
    -lrcl_interfaces__rosidl_typesupport_introspection_cpp
    -lrmw_fastrtps_cpp -lrmw_fastrtps_shared_cpp -lrmw_fastrtps_dynamic_cpp
    -lrmw_implementation -lrmw -lament_index_cpp -lclass-loader
    -lbuiltin_interfaces__rosidl_generator_c
    -lbuiltin_interfaces__rosidl_typesupport_c
    -lbuiltin_interfaces__rosidl_typesupport_cpp
    -lbuiltin_interfaces__rosidl_typesupport_fastrtps_c
    -lbuiltin_interfaces__rosidl_typesupport_fastrtps_cpp
    -lbuiltin_interfaces__rosidl_typesupport_introspection_c
    -lbuiltin_interfaces__rosidl_typesupport_introspection_cpp
    -lrcl-yaml-param-parser -lrcl_logging_interface -lrcl_logging_noop
    -lrosgraph_msgs__rosidl_generator_c
    -lrosgraph_msgs__rosidl_typesupport_c
    -lrosgraph_msgs__rosidl_typesupport_cpp
    -lrosgraph_msgs__rosidl_typesupport_fastrtps_c
    -lrosgraph_msgs__rosidl_typesupport_fastrtps_cpp
    -lrosgraph_msgs__rosidl_typesupport_introspection_c
    -lrosgraph_msgs__rosidl_typesupport_introspection_cpp
    -lrosidl_typesupport_c -lrosidl_typesupport_fastrtps_cpp
    -lrosidl_typesupport_cpp -lrosidl_typesupport_fastrtps_c
    -lrosidl_typesupport_introspection_c -lrosidl_typesupport_introspection_cpp
    -lstd_msgs__rosidl_generator_c
    -lstd_msgs__rosidl_typesupport_c
    -lstd_msgs__rosidl_typesupport_cpp
    -lstd_msgs__rosidl_typesupport_fastrtps_c
    -lstd_msgs__rosidl_typesupport_fastrtps_cpp
    -lstd_msgs__rosidl_typesupport_introspection_c
    -lstd_msgs__rosidl_typesupport_introspection_cpp
    -lrmw_dds_common -lrmw_dds_common__rosidl_generator_c
    -lrmw_dds_common__rosidl_typesupport_c
    -lrmw_dds_common__rosidl_typesupport_cpp
    -lrmw_dds_common__rosidl_typesupport_fastrtps_c
    -lrmw_dds_common__rosidl_typesupport_fastrtps_cpp
    -lrmw_dds_common__rosidl_typesupport_introspection_c
    -lrmw_dds_common__rosidl_typesupport_introspection_cpp
    -lrosidl_runtime_c
    -lfastrtps -lfastcdr -lfoonathan_memory -ltinyxml2
    -lrcpputils -lyaml -lrcutils
    -lstdc++ -lrtemscpu -lrtemsbsp -lrtemsdefaultconfig -lm -lgcc -lc -lbsd
    -Wl,--end-group
)

$CC \
    -march=armv7-a -mthumb -mfpu=neon -mfloat-abi=hard -mtune=cortex-a9 \
    -isystem"$BSP_LIB/include" \
    -B"$BSP_LIB" -qrtems \
    -Wl,--gc-sections -static \
    "$OBJ_FILE" \
    -o "$OUTPUT" \
    -L"$BSP_LIB" \
    "${LIBS[@]}"

if [ $? -eq 0 ]; then
    echo "SUCCESS: $OUTPUT created ($(du -h "$OUTPUT" | cut -f1))"
else
    echo "FAILED: Linking error"
    exit 1
fi
