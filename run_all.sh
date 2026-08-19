#!/bin/bash
#
# run_all.sh — Run all Exp_Inter experiments and collect results
# Usage: bash run_all.sh
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORKSPACE="/home/siweimutong/workspace"
BUILD_SUBDIR="build/arm-rtems6-realview_pbx_a9_qemu"
RESULT_DIR="$SCRIPT_DIR/Results-$(date +%Y%m%d-%H%M%S)"
QEMU_TIMEOUT=300  # 5 min per experiment

declare -A VARIANTS
VARIANTS=(
  ["NE"]="exp3inter_ne"
  ["ST"]="exp3inter_st"
  ["RT"]="exp3inter_rt"
  ["MT"]="exp3inter_mt"
  ["Picas"]="exp3inter_picas"
  ["Ours"]="exp3inter_ours"
)

# CSV tag patterns for each variant
declare -A CSV_TAGS
CSV_TAGS=(
  ["NE"]="E3INE-CSV"
  ["ST"]="E3IST-CSV"
  ["RT"]="E3IRT-CSV"
  ["MT"]="E3IMT-CSV"
  ["Picas"]="E3IP-CSV"
  ["Ours"]="E3OURS-CSV"
)

CHAIN_LENGTHS=(2 3 4 5 10 15)

# ---- Cleanup leftover QEMU processes from previous (killed) runs ----
cleanup_qemu() {
    pkill -9 qemu-system-arm 2>/dev/null || true
    sleep 0.5
}
cleanup_qemu

mkdir -p "$RESULT_DIR"

echo "============================================="
echo "Exp_Inter Batch Runner"
echo "Variants: ${!VARIANTS[*]}"
echo "Chain lengths: ${CHAIN_LENGTHS[*]}"
echo "Output: $RESULT_DIR"
echo "============================================="

# Total count
TOTAL=$((${#VARIANTS[@]} * ${#CHAIN_LENGTHS[@]}))
CURRENT=0

for VARIANT in NE ST RT MT Picas Ours; do
  PREFIX="${VARIANTS[$VARIANT]}"
  TAG="${CSV_TAGS[$VARIANT]}"
  VARIANT_DIR="$WORKSPACE/Example/Exp_Inter-$VARIANT"

  for N in "${CHAIN_LENGTHS[@]}"; do
    CURRENT=$((CURRENT + 1))
    EXE="$VARIANT_DIR/$BUILD_SUBDIR/${PREFIX}_n${N}.exe"
    RAW="$RESULT_DIR/${PREFIX}_n${N}_raw.txt"
    CSV="$RESULT_DIR/${PREFIX}_n${N}.csv"

    if [ ! -f "$EXE" ]; then
      echo "[$CURRENT/$TOTAL] SKIP $VARIANT n=$N (missing: $EXE)"
      continue
    fi

    echo -n "[$CURRENT/$TOTAL] Running $VARIANT n=$N ... "

    timeout $QEMU_TIMEOUT qemu-system-arm \
      -no-reboot -nographic -M realview-pbx-a9 -m 256M \
      -audiodev none,id=none \
      -semihosting \
      -kernel "$EXE" 2>/dev/null > "$RAW" || true

    # Extract CSV
    grep "^\[$TAG\]" "$RAW" | sed "s/^\[$TAG\] //" > "$CSV"
    LINES=$(wc -l < "$CSV" 2>/dev/null || echo 0)

    # Extract summary if present
    SUMMARY=$(grep "SUMMARY" "$RAW" | tail -1 || echo "")

    echo "${LINES} data rows"

    # Quick stats
    if [ "$LINES" -gt 0 ]; then
      E2E_COL=3
      AVG_E2E=$(awk -F',' -v col=$E2E_COL '{sum+=$col; count++} END {if(count>0) printf "%.0f", sum/count; else print "N/A"}' "$CSV")
      echo "       e2e_avg=${AVG_E2E}ns $SUMMARY"
    fi
  done
done

echo ""
echo "============================================="
echo "All experiments complete! Results in: $RESULT_DIR"
echo "============================================="

# Generate summary table
echo ""
echo "=== SUMMARY TABLE ==="
printf "%-8s" "Variant"
for N in "${CHAIN_LENGTHS[@]}"; do printf "  n=%-4s" "$N"; done
echo "  Executor"

for VARIANT in NE ST RT MT Picas Ours; do
  PREFIX="${VARIANTS[$VARIANT]}"
  printf "%-8s" "$VARIANT"
  for N in "${CHAIN_LENGTHS[@]}"; do
    CSV="$RESULT_DIR/${PREFIX}_n${N}.csv"
    if [ -f "$CSV" ]; then
      ROWS=$(wc -l < "$CSV" 2>/dev/null || echo 0)
      if [ "$ROWS" -gt 0 ]; then
        AVG=$(awk -F',' '{sum+=$3; count++} END {if(count>0) printf "%.0f", sum/count; else print "N/A"}' "$CSV")
        printf " %6s" "$AVG"
      else
        printf " %6s" "0row"
      fi
    else
      printf " %6s" "N/A"
    fi
  done
  case $VARIANT in
    NE) echo "  NoExecutor";;
    ST) echo "  SingleThreaded";;
    RT) echo "  RTExecutor";;
    MT) echo "  MultiThreaded";;
    Picas) echo "  PiCAS";;
    Ours) echo "  NoExecutor";;
  esac
done

echo ""
echo "Units: average e2e latency in nanoseconds"
