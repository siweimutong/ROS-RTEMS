#!/bin/bash
#
# run.sh — Build all and run on RTEMS QEMU
# Usage: ./run.sh <chain_length>
#         ./run.sh all    (run all 5)
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="build/arm-rtems6-realview_pbx_a9_qemu"
RESULT_DIR="Result"

mkdir -p "${RESULT_DIR}"

# Build if needed (skip when exes already exist)
[ -f "$BUILD_DIR/exp3inter_st_n2.exe" ] || ./build_all.sh

run_one() {
    local N="$1"
    local EXE="exp3inter_st_n${N}.exe"

    if [ ! -f "$BUILD_DIR/$EXE" ]; then
        echo "ERROR: $EXE not found"
        return 1
    fi

    echo "Running $EXE in QEMU..."
    local RAW="${RESULT_DIR}/exp3inter_st_n${N}_raw.txt"

    # Start QEMU in background for memory polling
    qemu-system-arm -no-reboot -nographic -M realview-pbx-a9 -m 256M \
        -audiodev none,id=none \
                -semihosting \
        -kernel "$BUILD_DIR/$EXE" > "$RAW" 2>/dev/null &
    local QPID=$!

    # Fallback timeout: kill QEMU after 600s if still running
    (sleep 600; kill -9 $QPID 2>/dev/null) &
    local TPID=$!

    trap 'pkill -9 qemu-system-arm 2>/dev/null || true' EXIT INT TERM
    # Poll for peak memory AND early-exit when program finishes (SUMMARY line)
    local PEAK_HWM=0 PEAK_VMSIZE=0 HWM VMSIZE
    local SUMMARY_SEEN=0
    while kill -0 $QPID 2>/dev/null; do
        if [ -f /proc/$QPID/status ]; then
            HWM=$(awk '/VmHWM/{print $2+0}' /proc/$QPID/status 2>/dev/null)
            VMSIZE=$(awk '/VmPeak/{print $2+0}' /proc/$QPID/status 2>/dev/null)
            [ -n "$HWM" ] && [ "$HWM" -gt "$PEAK_HWM" ] && PEAK_HWM=$HWM
            [ -n "$VMSIZE" ] && [ "$VMSIZE" -gt "$PEAK_VMSIZE" ] && PEAK_VMSIZE=$VMSIZE
        fi
        # Early-exit: once SUMMARY is written, give 2s grace then kill QEMU
        if [ "$SUMMARY_SEEN" -eq 0 ] && grep -q '\[E3IST-SUMMARY\]' "$RAW" 2>/dev/null; then
            SUMMARY_SEEN=1
            (sleep 2; kill -9 $QPID 2>/dev/null) &
        fi
        sleep 0.3
    done
    kill $TPID 2>/dev/null || true
    wait $QPID 2>/dev/null || true  # reap zombie

    # Record peak memory
    echo "exp3inter_st_n${N}  vmhwm_kb=${PEAK_HWM}  vmpeak_kb=${PEAK_VMSIZE}" >> "${RESULT_DIR}/memory_peaks.txt"

    # Extract CSV lines (header + data) into single csv file
    grep '^\[E3IST-CSV\]' "$RAW" | sed 's/^\[E3IST-CSV\] //' > "${RESULT_DIR}/exp3inter_st_n${N}.csv" || {
        echo "WARNING: no CSV lines found for n=${N}, checking raw output..."
        wc -l "$RAW" 2>/dev/null || true
    }

    local LINES=$(wc -l < "${RESULT_DIR}/exp3inter_st_n${N}.csv" 2>/dev/null || echo 0)
    echo "  n=${N}: ${LINES} lines  peak_rss=${PEAK_HWM}kB  peak_vm=${PEAK_VMSIZE}kB"
    if [ "$LINES" -gt 0 ]; then
        head -3 "${RESULT_DIR}/exp3inter_st_n${N}.csv"
    fi
}

if [ "${1:-all}" = "all" ]; then
    for n in 2 3 4 5; do
        run_one "$n"
    done
else
    run_one "$1"
fi
