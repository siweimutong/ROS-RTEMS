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
[ -f "$BUILD_DIR/exp2_ours_n2.exe" ] || ./build_all.sh

run_one() {
    local N="$1"
    local EXE="exp2_ours_n${N}.exe"

    if [ ! -f "$BUILD_DIR/$EXE" ]; then
        echo "ERROR: $EXE not found"
        return 1
    fi

    echo "Running $EXE in QEMU..."
    local RAW="${RESULT_DIR}/exp2_ours_n${N}_raw.txt"

    command -v qemu-system-arm >/dev/null 2>&1 || { echo "ERROR: qemu-system-arm not found"; exit 1; }

    # Suppress bash job-control "Killed" messages
    set +m
    # Start QEMU in background, output saved to file (2>&1 captures stderr too)
    qemu-system-arm -no-reboot -nographic -M realview-pbx-a9 -m 512M \
        -audiodev none,id=none \
                -semihosting \
        -kernel "$BUILD_DIR/$EXE" > "$RAW" 2>&1 &
    local QPID=$!
    set -m  # restore monitor mode

    # Fallback timeout: kill QEMU after 600s if still running
    (sleep 600; kill -9 $QPID 2>/dev/null) &
    local TPID=$!

    # Cleanup handler for abnormal exit only (interrupt / termination)
    trap 'kill -9 ${QPID:-} 2>/dev/null; wait ${QPID:-} 2>/dev/null; true' INT TERM

    # Poll for peak memory; QEMU exits by itself via semihosting SYS_EXIT
    local PEAK_HWM=0 PEAK_VMSIZE=0 AVG_RSS_SUM=0 AVG_RSS_COUNT=0 HWM VMSIZE RSS
    local PROGRESS_TICK=0
    while kill -0 $QPID 2>/dev/null; do
        if [ -f /proc/$QPID/status ]; then
            HWM=$(awk '/VmHWM/{print $2+0}' /proc/$QPID/status 2>/dev/null)
            VMSIZE=$(awk '/VmPeak/{print $2+0}' /proc/$QPID/status 2>/dev/null)
            RSS=$(awk '/VmRSS/{print $2+0}' /proc/$QPID/status 2>/dev/null)
            [ -n "$HWM" ] && [ "$HWM" -gt "$PEAK_HWM" ] && PEAK_HWM=$HWM
            [ -n "$VMSIZE" ] && [ "$VMSIZE" -gt "$PEAK_VMSIZE" ] && PEAK_VMSIZE=$VMSIZE
            if [ -n "$RSS" ] && [ "$RSS" -gt 0 ]; then
                AVG_RSS_SUM=$((AVG_RSS_SUM + RSS))
                AVG_RSS_COUNT=$((AVG_RSS_COUNT + 1))
            fi
        fi
        PROGRESS_TICK=$((PROGRESS_TICK + 1))
        if [ $((PROGRESS_TICK % 30)) -eq 0 ]; then
            printf "." >&2
        fi
        PROGRESS_TICK=$((PROGRESS_TICK + 1))
        if [ $((PROGRESS_TICK % 20)) -eq 0 ]; then
            printf "[%s] n=%s running... (peak_rss=%s kB)\n" "$(date +%H:%M:%S)" "${N}" "${PEAK_HWM}" >&2
        fi
        sleep 0.3
    done
    # Clear traps — QEMU exited normally
    trap - INT TERM
    echo "" >&2
    local AVG_RSS=0
    [ "$AVG_RSS_COUNT" -gt 0 ] && AVG_RSS=$((AVG_RSS_SUM / AVG_RSS_COUNT))
    kill $TPID 2>/dev/null || true
    # Reap zombie silently (suppress any "Killed" message)
    { wait $QPID 2>/dev/null; } 2>/dev/null || true

    # Record peak memory
    echo "exp2_ours_n${N}  vmhwm_kb=${PEAK_HWM}  vmpeak_kb=${PEAK_VMSIZE}  vmrss_avg_kb=${AVG_RSS}" >> "${RESULT_DIR}/memory_peaks.txt"

    # Extract CSV lines into single csv file
    grep '^\[E2OURS-CSV\]' "$RAW" | sed 's/^\[E2OURS-CSV\] //' > "${RESULT_DIR}/exp2_ours_n${N}.csv" || {
        echo "WARNING: no CSV lines found for n=${N}, checking raw output..."
        wc -l "$RAW" 2>/dev/null || true
    }

    local LINES=$(wc -l < "${RESULT_DIR}/exp2_ours_n${N}.csv" 2>/dev/null || echo 0)
    echo "  n=${N}: ${LINES} lines  peak_rss=${PEAK_HWM}kB  peak_vm=${PEAK_VMSIZE}kB"
    if [ "$LINES" -gt 0 ]; then
        head -3 "${RESULT_DIR}/exp2_ours_n${N}.csv"
    fi
    return 0
}

if [ "${1:-all}" = "all" ]; then
    for n in 2 3 4 5; do
        run_one "$n"
        sleep 2  # let QEMU resources fully release before next run
    done
else
    run_one "$1"
fi
