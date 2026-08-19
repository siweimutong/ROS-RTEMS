#!/bin/bash

target_dir="./tmp"
build_modle=$1
flag=0

root_path=$2
log_path="$root_path/build/logs"
log_file_path="$log_path/compile_log_yaml.txt"

if [[ ! -d "$log_path" ]]; then
    mkdir -p "$log_path"  # -pis key: creates build even if no subdirectories exist under root_path
fi

# Check whether the file exists; create it if not found
if [[ ! -f "$log_file_path" ]]; then
    touch "$log_file_path"  # Create empty file (if directory exists)
fi

# Key logic: -d detects whether a path is an existing directory
if [ -d "${target_dir}" ]; then
    # rm -rf Force recursive deletedirectory(Deletes files/subdirectories inside, no errors)
    if [[ "$build_modle" == "force" ]]; then
        rm -rf "${target_dir}"
        flag=1
    elif [[ "$build_modle" == "no-force" ]]; then
        flag=0
    else
        echo "build_modle arg error!!!"
        exit 1
    fi
else
    flag=1
fi

if [[ "$flag" == 1 ]]; then
    mkdir tmp
    cd tmp
    curl -s https://waf.io/waf-2.0.19 > waf
    chmod +x ./waf
    git init >> $log_file_path 2>&1
    GIT_CMD="git submodule add https://gitlab.rtems.org/rtems/tools/rtems_waf.git rtems_waf >> $log_file_path 2>&1"

    # Execute command and capture result
    if eval $GIT_CMD; then
        echo "✅ Submodule rtems_waf added successfully!" >> $log_file_path 2>&1
    else
        # Capture the failure, output error info, and exit
        ERROR_CODE=$?  # Get command exit code
        echo "❌ Submodule rtems_waf add failed! Exit code: ${ERROR_CODE}" >> $log_file_path 2>&1
        echo "❌ Failed possibly because: network issue / repository not found / path already exists / insufficient permissions" >> $log_file_path 2>&1
        
        # Optional: output detailed error logs (to help diagnose issues)
        echo "📝 Detailed error log: " >> $log_file_path 2>&1
        $GIT_CMD 2>&1  # Re-execute and output full error info (2>&1 redirects stderr to stdout)
        
        exit $ERROR_CODE  # Exit with non-zero code to notify caller of failure
    fi

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

# python ../mk_to_wscript_static.py ../libyaml.mk wscript

    touch wscript
    echo '
#!/usr/bin/env python
# A Waf script to build an RTEMS project with static libraries

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
    # Here you can add BSP-specific configuration checks
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure=bsp_configure)

def build(bld):
    rtems.build(bld)

    # yaml target (static library)
    # Compile and link static library
    bld.stlib(
        target="yaml",
        source=[
            "init.c",
            "../../code/src/libyaml-0.2.5/src/api.c",
            "../../code/src/libyaml-0.2.5/src/dumper.c",
            "../../code/src/libyaml-0.2.5/src/emitter.c",
            "../../code/src/libyaml-0.2.5/src/loader.c",
            "../../code/src/libyaml-0.2.5/src/parser.c",
            "../../code/src/libyaml-0.2.5/src/reader.c",
            "../../code/src/libyaml-0.2.5/src/scanner.c",
            "../../code/src/libyaml-0.2.5/src/writer.c"
        ],
        includes=[
            "../../code/src/libyaml-0.2.5/include",
            "../../../DDS/tinyxml2/code/include"
        ],
        defines=[
            "RMW_IMPLEMENTATION=rmw_fastrtps_cpp",
        ],
        cxxflags="-DRTEMS  -fPIC",
        linkflags="-static",
        lib=["stdc++", "rtemscpu", "rtemsbsp", "rtemsdefaultconfig", "m", "gcc", "c", "tinyxml2"],
    )
' > wscript
fi

# Always auto-generate config.h (needed for both force and no-force)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
YAML_SRC_DIR="$SCRIPT_DIR/../code/src/libyaml-0.2.5"
CONFILE="$YAML_SRC_DIR/configure.ac"
YAML_MAJOR=$(grep 'm4_define(\[YAML_MAJOR\]' "$CONFILE" | grep -o ', *[0-9]*' | grep -o '[0-9]*')
YAML_MINOR=$(grep 'm4_define(\[YAML_MINOR\]' "$CONFILE" | grep -o ', *[0-9]*' | grep -o '[0-9]*')
YAML_PATCH=$(grep 'm4_define(\[YAML_PATCH\]' "$CONFILE" | grep -o ', *[0-9]*' | grep -o '[0-9]*')
CONFIG_H_PATH="$YAML_SRC_DIR/include/config.h"
echo "/* Auto-generated config.h for RTEMS cross-compilation */
#define YAML_VERSION_MAJOR ${YAML_MAJOR}
#define YAML_VERSION_MINOR ${YAML_MINOR}
#define YAML_VERSION_PATCH ${YAML_PATCH}
#define YAML_VERSION_STRING \"${YAML_MAJOR}.${YAML_MINOR}.${YAML_PATCH}\"" > ${CONFIG_H_PATH}
echo "✅ config.h auto-generated (v${YAML_MAJOR}.${YAML_MINOR}.${YAML_PATCH})" >> $log_file_path 2>&1

exit 0