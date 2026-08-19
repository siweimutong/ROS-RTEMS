#!/bin/bash
#
# run_all_exp_inter.sh — Run sequentially Exp_Inter-ST, Exp_Inter-MT, Exp_Inter-Picas, Exp_Inter-ROSRT, Exp_Inter-Ours
#
# Run from host; launches QEMU inside the rtems6-dev container via docker exec
#
set -euo pipefail

CONTAINER="rtems6-dev"
WORKSPACE="/home/siweimutong/workspace"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

EXPERIMENTS=("Example/Exp_Inter-ST" "Example/Exp_Inter-MT" "Example/Exp_Inter-Picas" "Example/Exp_Inter-ROSRT" "Example/Exp_Inter-Ours")

# ---- Cleanup leftover QEMU processes from previous (killed) runs ----
cleanup_qemu() {
    docker exec -i "$CONTAINER" bash -lc \
        'pkill -9 qemu-system-arm 2>/dev/null; pkill -9 -f "timeout.*qemu" 2>/dev/null; true' || true
    sleep 0.5
}

if [ "$(docker inspect -f '{{.State.Running}}' "$CONTAINER" 2>/dev/null)" != "true" ]; then
    echo -e "${RED}Error: Container $CONTAINER is not running${NC}"
    exit 1
fi

echo ""
echo "============================================================"
echo -e "  ${CYAN}Exp_Inter Batch Run${NC}"
echo "  Container: $CONTAINER"
echo "  Experiments: ${EXPERIMENTS[*]}"
echo "  Start time: $(date '+%Y-%m-%d %H:%M:%S')"
echo "============================================================"
echo ""

FAILED=()
PASSED=()
START_ALL=$(date +%s)

cleanup_qemu

for exp in "${EXPERIMENTS[@]}"; do
    cleanup_qemu
    echo -e "${CYAN}┌──────────────────────────────────────────────────────────┐${NC}"
    echo -e "${CYAN}│  Running: ${exp}  (estimated ~30 minutes, 6 chain lengths)${NC}"
    echo -e "${CYAN}└──────────────────────────────────────────────────────────┘${NC}"

    EXP_START=$(date +%s)

    if ! docker exec -i "$CONTAINER" bash -lc \
        "test -f ${WORKSPACE}/${exp}/run.sh"; then
        echo -e "  ${RED}Error: ${exp}/run.sh not found${NC}"
        FAILED+=("${exp}")
        continue
    fi

    if docker exec -i "$CONTAINER" bash -lc \
        "cd ${WORKSPACE}/${exp} && ./run.sh all"; then
        EXP_ELAPSED=$(( $(date +%s) - EXP_START ))
        echo -e "  ${GREEN}✓ ${exp} Run complete (${EXP_ELAPSED}s)${NC}"
        PASSED+=("${exp}")
    else
        EXP_ELAPSED=$(( $(date +%s) - EXP_START ))
        echo -e "  ${RED}✗ ${exp} Run failed (${EXP_ELAPSED}s)${NC}"
        FAILED+=("${exp}")
    fi
    echo ""
done

TOTAL_ALL=$(( $(date +%s) - START_ALL ))

echo "============================================================"
echo -e "  ${YELLOW}Run summary${NC}"
echo "  Total time: ${TOTAL_ALL}s"
echo -e "  Success (${#PASSED[@]}): ${GREEN}${PASSED[*]:-(none)}${NC}"
echo -e "  Failed (${#FAILED[@]}): ${RED}${FAILED[*]:-(none)}${NC}"
echo "============================================================"
echo ""

[ ${#FAILED[@]} -gt 0 ] && exit 1
exit 0
