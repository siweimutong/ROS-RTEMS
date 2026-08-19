#!/bin/bash

cd tmp

rm -rf ./.waf*
rm -rf ./build

./waf configure --rtems=$HOME/RTEMS/quick-start/rtems/6 --rtems-bsp=arm/realview_pbx_a9_qemu
./waf build