#!/bin/bash
#
# build_all_exp1.sh — Build sequentially Exp1-ST, Exp1-Picas, Exp1-ROSRT, Exp1-Ours
#
# 从宿主机Execute,通过 docker exec 在 rtems6-dev Container内Build
#
set -euo pipefail

CONTAINER="rtems6-dev"
WORKSPACE="/home/siweimutong/workspace"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

EXPERIMENTS=("Example/Exp1-ST" "Example/Exp1-Picas" "Example/Exp1-ROSRT" "Example/Exp1-Ours")

if [ "$(docker inspect -f '{{.State.Running}}' "$CONTAINER" 2>/dev/null)" != "true" ]; then
    echo -e "${RED}Error: Container $CONTAINER is not running${NC}"
    exit 1
fi

echo ""
echo "============================================================"
echo -e "  ${CYAN}Exp1 Batch Build${NC}"
echo "  Experiments: ${EXPERIMENTS[*]}"
echo "  Start time: $(date '+%Y-%m-%d %H:%M:%S')"
echo "============================================================"
echo ""

FAILED=()
PASSED=()
START_ALL=$(date +%s)

for exp in "${EXPERIMENTS[@]}"; do
    echo -e "${CYAN}┌──────────────────────────────────────────────────────────┐${NC}"
    echo -e "${CYAN}│  Building: ${exp}${NC}"
    echo -e "${CYAN}└──────────────────────────────────────────────────────────┘${NC}"
    EXP_START=$(date +%s)

    if ! docker exec -i "$CONTAINER" bash -lc \
        "test -f ${WORKSPACE}/${exp}/build_all.sh"; then
        echo -e "  ${RED}Error: ${exp}/build_all.sh not found${NC}"
        FAILED+=("${exp}")
        continue
    fi

    if docker exec -i "$CONTAINER" bash -lc \
        "cd ${WORKSPACE}/${exp} && ./build_all.sh"; then
        EXP_ELAPSED=$(( $(date +%s) - EXP_START ))
        echo -e "  ${GREEN}✓ ${exp} Build succeeded (${EXP_ELAPSED}s)${NC}"
        PASSED+=("${exp}")
    else
        EXP_ELAPSED=$(( $(date +%s) - EXP_START ))
        echo -e "  ${RED}✗ ${exp} Build failed (${EXP_ELAPSED}s)${NC}"
        FAILED+=("${exp}")
    fi
    echo ""
done

TOTAL_ALL=$(( $(date +%s) - START_ALL ))
echo "============================================================"
echo -e "  ${YELLOW}Build summary${NC}"
echo "  Total time: ${TOTAL_ALL}s"
echo -e "  Success (${#PASSED[@]}): ${GREEN}${PASSED[*]:-(none)}${NC}"
echo -e "  Failed (${#FAILED[@]}): ${RED}${FAILED[*]:-(none)}${NC}"
echo "============================================================"
echo ""
[ ${#FAILED[@]} -gt 0 ] && exit 1
exit 0
