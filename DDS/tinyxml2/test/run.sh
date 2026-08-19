#!/bin/bash
cd tmp

rm -rf ./.waf*
rm -rf ./build

./waf configure --rtems=$HOME/RTEMS/quick-start/rtems/6 --rtems-bsp=arm/realview_pbx_a9_qemu
./waf build

qemu-system-arm -no-reboot -nographic -M realview-pbx-a9 -m 256M -kernel ./build/arm-rtems6-realview_pbx_a9_qemu/test_main.exe