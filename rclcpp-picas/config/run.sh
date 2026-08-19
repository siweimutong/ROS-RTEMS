#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"

# $1 = build_mode (force/no-force), $2 = workspace_root
BUILD_MODE="${1:-no-force}"
WORKSPACE_ROOT="${2:-$(cd ../../ && pwd)}"

log_path="$WORKSPACE_ROOT/build/logs"
log_file_path="$log_path/compile_log_rclcpp-picas.txt"

mkdir -p "$log_path"
touch "$log_file_path"

# Step 1: Setup (gen_run.sh creates wscript + downloads waf if needed)
bash gen_run.sh "$BUILD_MODE" "$WORKSPACE_ROOT" >> "$log_file_path" 2>&1

# Step 2: Clean if force mode
cd tmp
if [[ "$BUILD_MODE" == "force" ]]; then
    rm -rf ./.waf* ./build
fi

# Step 3: Configure + Build
./waf configure \
    --rtems="$HOME/RTEMS/quick-start/rtems/6" \
    --rtems-bsp=arm/realview_pbx_a9_qemu \
    >> "$log_file_path" 2>&1

./waf build -v >> "$log_file_path" 2>&1

# Step 4: Verify build output (kept in own build dir; PiCAS experiments link directly from here)
if [[ ! -f "$SOURCE_LIB" ]]; then
    echo "WARNING: $SOURCE_LIB not found after build" >> "$log_file_path"
    exit 1
fi

echo "PiCAS librclcpp.a built at $SOURCE_LIB" >> "$log_file_path"
echo "NOTE: System librclcpp.a NOT overwritten — PiCAS experiments link with -L<PICAS_BUILD_DIR>" >> "$log_file_path"

exit 0
