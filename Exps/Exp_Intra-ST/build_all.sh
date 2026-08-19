#!/bin/bash
#
# build_all.sh — Compile once, then link all 5 chain lengths for Exp_Intra-ST
# Uses patched rclcpp-16.0.8 (same as Exp_Intra-MT) for instrumentation symbols
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

RTEMS_PATH="/root/RTEMS/quick-start/rtems/6"
BSP="arm/realview_pbx_a9_qemu"
BUILD_DIR="build/arm-rtems6-realview_pbx_a9_qemu"
PATCHED_RCLCPP_DIR="../../rclcpp/config/config_rclcpp/tmp/build/arm-rtems6-realview_pbx_a9_qemu"

CC="$RTEMS_PATH/bin/arm-rtems6-g++"
BSP_LIB="$RTEMS_PATH/arm-rtems6/realview_pbx_a9_qemu/lib"

# Step 1: Compile with waf
echo "[1/2] Compiling all targets..."
python3 waf configure --rtems="$RTEMS_PATH" --rtems-bsp="$BSP" 2>/dev/null
python3 waf build 2>&1 | grep -v "undefined reference\|cannot find\|ld returned\|Build failed" || true

# Verify object files (two source files per target, indices 1..5)
IDX=1
for N in 2 3 4 5; do
    TEST_OBJ="$BUILD_DIR/src/test_main.cpp.${IDX}.o"
    MAIN_OBJ="$BUILD_DIR/src/exp2_st_main.cpp.${IDX}.o"
    if [ ! -f "$TEST_OBJ" ] || [ ! -f "$MAIN_OBJ" ]; then
        echo "ERROR: Object files not found for n=$N (idx=$IDX)"
        exit 1
    fi
    IDX=$((IDX + 1))
done

# Step 2: Link all 5 executables (using patched rclcpp-16.0.8)
LIBS=(
    -Wl,--start-group
    "$PATCHED_RCLCPP_DIR/librclcpp.a"
    -lrcl -lstatistics_collector
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

echo "[2/2] Linking all executables (using patched rclcpp-16.0.8)..."
IDX=1
for N in 2 3 4 5; do
    TEST_OBJ="$BUILD_DIR/src/test_main.cpp.${IDX}.o"
    MAIN_OBJ="$BUILD_DIR/src/exp2_st_main.cpp.${IDX}.o"
    OUTPUT="$BUILD_DIR/exp2_st_n${N}.exe"

    $CC \
        -march=armv7-a -mthumb -mfpu=neon -mfloat-abi=hard -mtune=cortex-a9 \
        -isystem"$BSP_LIB/include" \
        -B"$BSP_LIB" -qrtems \
        -Wl,--gc-sections -static \
        "$TEST_OBJ" "$MAIN_OBJ" \
        -o "$OUTPUT" \
        -L"$BSP_LIB" \
        -L"$PATCHED_RCLCPP_DIR" \
        "${LIBS[@]}"

    if [ $? -eq 0 ]; then
        echo "  OK: $OUTPUT ($(du -h "$OUTPUT" | cut -f1))"
    else
        echo "  FAILED: $OUTPUT"
        exit 1
    fi
    IDX=$((IDX + 1))
done

echo "All 5 executables built successfully."
