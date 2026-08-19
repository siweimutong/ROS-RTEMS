#!/bin/bash
BSP="arm/realview_pbx_a9_qemu"
RTEMS_PATH="/home/siweimutong/rtems/rtros/rtems/6"
BUILD_DIR="build/arm-rtems6-realview_pbx_a9_qemu"
EXE="rtexecutor.exe"
case "$1" in
    clean) python3 waf clean ;;
    run)
        python3 waf configure --rtems="$RTEMS_PATH" --rtems-bsp="$BSP" 2>/dev/null
        python3 waf build
        if [ $? -eq 0 ]; then
            echo "Running $EXE in QEMU..."
            qemu-system-arm -M realview-pbx-a9 -m 256M -nographic -audiodev none,id=none \
                -kernel "$BUILD_DIR/$EXE"
        fi ;;
    *) python3 waf configure --rtems="$RTEMS_PATH" --rtems-bsp="$BSP" 2>/dev/null
       python3 waf build ;;
esac
