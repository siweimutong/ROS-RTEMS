#!/bin/bash

target_dir="./tmp"

# Key logic: -d detects whether a path is an existing directory
if [ -d "${target_dir}" ]; then
    # rm -rf Force recursive deletedirectory(Deletes files/subdirectories inside, no errors)
    rm -rf "${target_dir}"
    echo "tmp directory in current directory existed; successfully deleted"
else
    echo "当前directory下not foundtmpdirectory,noneneedDelete"
fi

mkdir tmp
cd tmp
curl https://waf.io/waf-2.0.19 > waf
chmod +x ./waf
git init
git submodule add https://gitlab.rtems.org/rtems/tools/rtems_waf.git rtems_waf

touch wscript
echo '
#
# Hello world Waf script
#
from __future__ import print_function

rtems_version = "6"

try:
    import rtems_waf.rtems as rtems
except:
    print("error: no rtems_waf git submodule")
    import sys
    sys.exit(1)

def init(ctx):
    rtems.init(ctx, version = rtems_version, long_commands = True)

def bsp_configure(conf, arch_bsp):
    # Add BSP specific configuration checks
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure = bsp_configure)


def build(bld):
    rtems.build(bld)

    bld.program(
            features = "cxx",
            target = "test_main.exe",
            source = [
                "../HelloWorld_main.cpp",
                "../HelloWorld.cpp",
                "../HelloWorldPublisher.cpp",
                "../HelloWorldPubSubTypes.cpp",
                "../HelloWorldSubscriber.cpp"
            ],
            includes = [
                "../../../../../../tinyxml2/code/include",
                "../../../Fast-CDR/code/include",
                "../../../Fast-CDR/code/build/include",
                "../../../Fast-DDS/code/include",
                "../../../Fast-DDS/code/build/include",
            ],
            defines=[
                "RTEMS",
                "_DEFAULT_SOURCE=1"
            ],
            cflags="-Wno-psabi -Wall -Wmissing-prototypes -Wimplicit-function-declaration -Wstrict-prototypes -Wnested-externs -O0 -fdata-sections -ffunction-sections -march=armv7-a -mthumb -mfpu=neon -mfloat-abi=hard -mtune=cortex-a9 -fpermissive -fexceptions -frtti -std=c++17 -Wno-register -g -Wno-unknown-pragmas -fPIC",
            cxxflags="-Wno-psabi -Wall -O0 -fdata-sections -ffunction-sections -march=armv7-a -mthumb -mfpu=neon -mfloat-abi=hard -mtune=cortex-a9 -fpermissive -fexceptions -frtti -std=c++17 -Wno-register -g -Wno-unknown-pragmas -fPIC",
            lib=["rtemscpu", "rtemsbsp", "rtemsdefaultconfig", "stdc++", "m", "gcc", "c", "fastrtps", "fastcdr", "foonathan_memory", "tinyxml2", "bsd"],
            linkflags=["-static", "-Wl,-Bstatic"],
        )
' > wscript