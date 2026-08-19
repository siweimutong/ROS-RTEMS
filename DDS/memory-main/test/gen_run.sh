#!/bin/bash

target_dir="./tmp"

# Key logic: -d detects whether a path is an existing directory
if [ -d "${target_dir}" ]; then
    # rm -rf Force recursive deletedirectory(Deletes files/subdirectories inside, no errors)
    rm -rf "${target_dir}"
    echo "tmp directory in current directory existed; successfully deleted"
else
    echo "No tmp directory found in the current directory; nothing to delete"
fi

mkdir tmp
cd tmp
curl https://waf.io/waf-2.0.19 > waf
chmod +x ./waf
git init
git submodule add https://gitlab.rtems.org/rtems/tools/rtems_waf.git rtems_waf

touch init.c
echo "/*
 * Simple RTEMS configuration
 */

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT

#include <rtems/confdefs.h>" > init.c

touch wscript
echo '
#!/usr/bin/env python  
# A Waf script to build an RTEMS project including a C++ library and executable  

from __future__ import print_function  

rtems_version = "6"  

try:  
    import rtems_waf.rtems as rtems  
except ImportError:  
    print("error: no rtems_waf git submodule")  
    import sys  
    sys.exit(1)  

def init(ctx):  
    rtems.init(ctx, version=rtems_version, long_commands=True)
    ctx.load("compiler_cxx")  # Ensure C++ compiler is loaded

def bsp_configure(conf, arch_bsp):  
    # BSP-related configuration checks can be added here  
    pass  

def options(opt):  
    rtems.options(opt)  

def configure(conf):  
    rtems.configure(conf, bsp_configure=bsp_configure)  
    
    # conf.env.append_value("libpath", "/home/yzx/RTEMS/quick-start/app/workspace/DDS/tinyxml2/config/tmp/bulid/arm-rtems6-realview_pbx_a9_qemu/") # Append path

def build(bld):  
    rtems.build(bld)  

    # Compile and link static library  
    bld.program(
        target="test_main.exe",                 # Output library name  
        source=["init.c", "../test_main.cpp"],  # Source file  
        includes=["../../code/include", "../../code/include/foonathan/memory", "../../code/test"], #Header files
        cxxflags="-Wno-psabi -fpermissive -fexceptions -frtti -std=c++17 -Wno-register -g -Wno-unknown-pragmas -fPIC",
        lib=["rtemscpu", "stdc++", "rtemsbsp", "rtemsdefaultconfig", "m", "gcc", "c", "supc++", "foonathan_memory"],
        linkflags=["-static", "-Wl,-Bstatic"],
    )  
' > wscript