#!/bin/bash
# Build and run RT-ROS (EventsCBGExecutor) Benchmark
#
# Usage:
#   ./run.sh           # Build only
#   ./run.sh run       # Build and run in QEMU
#   ./run.sh clean     # Clean build

BSP="arm/realview_pbx_a9_qemu"
RTEMS_PATH="/home/siweimutong/docker_ws/rtems/rtros/rtems/6"
BUILD_DIR="build/arm-rtems6-realview_pbx_a9_qemu"
EXE="rtros.exe"

case "$1" in
    clean)
        python3 waf clean
        ;;
    run)
        # Build first
        python3 waf configure --rtems="$RTEMS_PATH" --rtems-bsp="$BSP" 2>/dev/null
        python3 waf build
        if [ $? -eq 0 ]; then
            echo "Running $EXE in QEMU..."
            qemu-system-arm -M realview-pbx-a9 -m 256M -nographic \
                -kernel "$BUILD_DIR/$EXE"
        fi
        ;;
    *)
        python3 waf configure --rtems="$RTEMS_PATH" --rtems-bsp="$BSP" 2>/dev/null
        python3 waf build
        echo ""
        echo "Build complete. Binary: $BUILD_DIR/$EXE"
        echo "To run: qemu-system-arm -M realview-pbx-a9 -m 256M -nographic -kernel $BUILD_DIR/$EXE"
        ;;
esac
