#!/bin/bash
#
# run_all_exp.sh — 依次执行全部实验: Exp_Intra → Exp_Inter
# 每个脚本完全退出后再执行下一个
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONTAINER="rtems6-dev"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'

[ "$(docker inspect -f '{{.State.Running}}' "$CONTAINER" 2>/dev/null)" = "true" ] || {
    echo -e "${RED}Container $CONTAINER is not running${NC}"; exit 1
}

# ---- Cleanup leftover QEMU processes from previous (killed) runs ----
cleanup_qemu() {
    docker exec -i "$CONTAINER" bash -lc \
        'pkill -9 qemu-system-arm 2>/dev/null; pkill -9 -f "timeout.*qemu" 2>/dev/null; true' || true
    sleep 0.5
}
cleanup_qemu

STEPS=(
    "run_all_exp_intra.sh"
    "run_all_exp_inter.sh"
)

TOTAL=${#STEPS[@]}
START_TIME=$(date '+%Y-%m-%d %H:%M:%S')

echo ""
echo "============================================================"
echo -e "  ${CYAN}All Experiments Batch Run${NC}"
echo "  Steps: ${STEPS[*]}"
echo "  Start: ${START_TIME}"
echo "============================================================"
echo ""

FAILED=()
for i in "${!STEPS[@]}"; do
    step="${STEPS[$i]}"
    idx=$((i + 1))
    echo -e "${CYAN}┌──────────────────────────────────────────────────────────┐${NC}"
    echo -e "${CYAN}│  [${idx}/${TOTAL}]  Running: ${step}${NC}"
    echo -e "${CYAN}└──────────────────────────────────────────────────────────┘${NC}"
    echo "  $(date '+%Y-%m-%d %H:%M:%S')"

    if [ -x "${SCRIPT_DIR}/${step}" ]; then
        if "${SCRIPT_DIR}/${step}"; then
            echo -e "  ${GREEN}✓ [${idx}/${TOTAL}] ${step} done${NC}"
        else
            echo -e "  ${RED}✗ [${idx}/${TOTAL}] ${step} failed (exit code $?)${NC}"
            FAILED+=("${step}")
        fi
    else
        echo -e "  ${RED}✗ [${idx}/${TOTAL}] ${step}: script not found or not executable${NC}"
        FAILED+=("${step}")
    fi
    echo ""
done

END_TIME=$(date '+%Y-%m-%d %H:%M:%S')

echo "============================================================"
echo -e "  Start:  ${START_TIME}"
echo -e "  End:    ${END_TIME}"
if [ ${#FAILED[@]} -eq 0 ]; then
    echo -e "  ${GREEN}All ${TOTAL} steps completed successfully${NC}"
else
    echo -e "  ${RED}${#FAILED[@]}/${TOTAL} steps failed: ${FAILED[*]}${NC}"
fi
echo "============================================================"
echo ""
