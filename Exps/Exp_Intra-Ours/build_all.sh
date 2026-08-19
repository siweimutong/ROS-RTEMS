#!/bin/bash
#
# build_all.sh — Compile and link all 5 chain lengths for Exp_Intra-Ours
# Uses rclcpp-16.0.8 librclcpp.a (with RTExecutor)
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

RTEMS_PATH="/root/RTEMS/quick-start/rtems/6"
BSP="arm/realview_pbx_a9_qemu"
BUILD_DIR="build/arm-rtems6-realview_pbx_a9_qemu"

CC="$RTEMS_PATH/bin/arm-rtems6-g++"
BSP_LIB="$RTEMS_PATH/arm-rtems6/realview_pbx_a9_qemu/lib"

# rclcpp-16.0.8-built librclcpp.a (contains RTExecutor)
RCLCPP_LIB_DIR="../../rclcpp/config/config_rclcpp/tmp/build/arm-rtems6-realview_pbx_a9_qemu"

# Step 1: Compile with waf (linking will fail, but we get .o files)
echo "[1/2] Compiling all targets..."
python3 waf configure --rtems="$RTEMS_PATH" --rtems-bsp="$BSP" 2>/dev/null
python3 waf build >/dev/null 2>&1 || true

# Verify object files
IDX=1
for N in 2 3 4 5; do
    MAIN_OBJ="$BUILD_DIR/src/exp2_ours_main.cpp.${IDX}.o"
    if [ ! -f "$MAIN_OBJ" ]; then
        echo "ERROR: Object file not found for n=$N (idx=$IDX)"
        exit 1
    fi
    IDX=$((IDX + 1))
done

# Step 2: Link all 5 executables using rclcpp-16.0.8 librclcpp.a
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

echo "[2/2] Linking all executables (using rclcpp-16.0.8 librclcpp.a)..."
IDX=1
for N in 2 3 4 5; do
    MAIN_OBJ="$BUILD_DIR/src/exp2_ours_main.cpp.${IDX}.o"
    TEST_OBJ="$BUILD_DIR/src/test_main.cpp.${IDX}.o"
    IMPL_OBJ="$BUILD_DIR/src/rt_executor_impl.cpp.1.o"
    DRIVER_OBJ="$BUILD_DIR/driver/rtss_timer_driver.c.1.o"

    if [ ! -f "$MAIN_OBJ" ] || [ ! -f "$TEST_OBJ" ]; then
        echo "ERROR: Object files not found for n=$N (idx=$IDX)"
        exit 1
    fi

    OUTPUT="$BUILD_DIR/exp2_ours_n${N}.exe"

    $CC \
        -march=armv7-a -mthumb -mfpu=neon -mfloat-abi=hard -mtune=cortex-a9 \
        -isystem"$BSP_LIB/include" \
        -B"$BSP_LIB" -qrtems \
        -Wl,--gc-sections -static \
        "$MAIN_OBJ" "$TEST_OBJ" "$IMPL_OBJ" "$DRIVER_OBJ" \
        -o "$OUTPUT" \
        -L"$RCLCPP_LIB_DIR" \
        -L"$BSP_LIB" \
        "${LIBS[@]}"

    if [ $? -eq 0 ]; then
        echo "  OK: $OUTPUT ($(du -h "$OUTPUT" | cut -f1))"
    else
        echo "  FAILED: $OUTPUT"
        exit 1
    fi
    IDX=$((IDX + 1))
done

echo "All 4 executables built successfully."
