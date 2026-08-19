#!/bin/bash
# run_compare.sh — Run all working Exp_Inter variants and compare
set -euo pipefail

WORKSPACE=/home/siweimutong/workspace
BUILD=build/arm-rtems6-realview_pbx_a9_qemu
RESULT=$WORKSPACE/Results-compare-$(date +%Y%m%d-%H%M%S)
mkdir -p "$RESULT"
QEMU_TIMEOUT=300

declare -A CFG
CFG["NE"]="exp3inter_ne|E3INE-CSV|NoExecutor"
CFG["RT"]="exp3inter_rt|E3IRT-CSV|RTExecutor(rtss25)"
CFG["MT"]="exp3inter_mt|E3IMT-CSV|MultiThreaded"
CFG["Picas"]="exp3inter_picas|E3IP-CSV|PiCAS"
CFG["Ours"]="exp3inter_ours|E3OURS-CSV|RTExecutor(rtems)"

N_VALUES=(2 3 4 5 10 15)
TOTAL=$((5 * 6))
CURRENT=0

echo "=== Exp_Inter Full Comparison ==="
echo "Variants: ${!CFG[*]}"
echo "Chain lengths: ${N_VALUES[*]}"
echo "Output: $RESULT"
echo ""

for v in NE RT MT Picas Ours; do
  IFS='|' read -r prefix tag desc <<< "${CFG[$v]}"
  echo "--- $v ($desc) ---"
  for n in "${N_VALUES[@]}"; do
    CURRENT=$((CURRENT + 1))
    exe="$WORKSPACE/Example/Exp_Inter-$v/$BUILD/${prefix}_n${n}.exe"
    raw="$RESULT/${prefix}_n${n}_raw.txt"
    csv="$RESULT/${prefix}_n${n}.csv"
    echo -n "  [$CURRENT/$TOTAL] n=$n ... "
    timeout $QEMU_TIMEOUT qemu-system-arm -no-reboot -nographic \
      -M realview-pbx-a9 -m 256M -audiodev none,id=none \
      -kernel "$exe" 2>/dev/null > "$raw" || true
    grep "^\[$tag\]" "$raw" | sed "s/^\[$tag\] //" > "$csv"
    lines=$(wc -l < "$csv")
    if [ "$lines" -gt 1 ]; then
      e2e=$(awk -F',' 'NR>1 {sum+=$3; c++} END {printf "%.0f", sum/c}' "$csv")
      pub=$(awk -F',' 'NR>1 {sum+=$4; c++} END {printf "%.0f", sum/c}' "$csv")
      sched=$(awk -F',' 'NR>1 {sum+=$9; c++} END {printf "%.0f", sum/c}' "$csv")
      echo "OK e2e=${e2e}ns pub=${pub}ns sched=${sched}ns"
    else
      echo "FAILED ($lines data rows)"
    fi
  done
done

echo ""
echo "=== RESULTS ==="
printf "%-8s %-18s" "Var" "Executor"
for n in "${N_VALUES[@]}"; do printf "  n=%-6s" "$n"; done
echo ""

for v in NE RT MT Picas Ours; do
  IFS='|' read -r prefix tag desc <<< "${CFG[$v]}"
  printf "%-8s %-18s" "$v" "$desc"
  for n in "${N_VALUES[@]}"; do
    csv="$RESULT/${prefix}_n${n}.csv"
    if [ -f "$csv" ]; then
      lines=$(wc -l < "$csv")
      if [ "$lines" -gt 1 ]; then
        e2e=$(awk -F',' 'NR>1 {sum+=$3; c++} END {printf "%.0f", sum/c}' "$csv")
        printf " %8s" "$e2e"
      else
        printf " %8s" "FAIL"
      fi
    else
      printf " %8s" "N/A"
    fi
  done
  echo ""
done

echo ""
echo "Units: nanoseconds (e2e average latency)"
echo "Results saved in: $RESULT"
