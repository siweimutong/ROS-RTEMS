#!/bin/bash
#
# build_all_exp5.sh — Build sequentially Exp5-ST, Exp5-MT, Exp5-Picas, Exp5-ROSRT, Exp5-Ours
#
set -euo pipefail
CONTAINER="rtems6-dev"; WORKSPACE="/home/siweimutong/workspace"
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'
EXPERIMENTS=("Example/Exp5-ST" "Example/Exp5-MT" "Example/Exp5-Picas" "Example/Exp5-ROSRT" "Example/Exp5-Ours")

[ "$(docker inspect -f '{{.State.Running}}' "$CONTAINER" 2>/dev/null)" = "true" ] || { echo -e "${RED}Container $CONTAINER is not running${NC}"; exit 1; }

echo ""; echo "============================================================"
echo -e "  ${CYAN}Exp5 Batch Build${NC}"; echo "  Start time: $(date '+%Y-%m-%d %H:%M:%S')"
echo "============================================================"; echo ""
FAILED=(); PASSED=(); START_ALL=$(date +%s)

for exp in "${EXPERIMENTS[@]}"; do
    echo -e "${CYAN}┌──────────────────────────────────────────────────────────┐${NC}"
    echo -e "${CYAN}│  Building: ${exp}${NC}"
    echo -e "${CYAN}└──────────────────────────────────────────────────────────┘${NC}"
    EXP_START=$(date +%s)
    if docker exec -i "$CONTAINER" bash -lc "test -f ${WORKSPACE}/${exp}/build_all.sh" && \
       docker exec -i "$CONTAINER" bash -lc "cd ${WORKSPACE}/${exp} && ./build_all.sh"; then
        echo -e "  ${GREEN}✓ ${exp} Build succeeded ($(( $(date +%s) - EXP_START ))s)${NC}"; PASSED+=("${exp}")
    else
        echo -e "  ${RED}✗ ${exp} Build failed ($(( $(date +%s) - EXP_START ))s)${NC}"; FAILED+=("${exp}")
    fi; echo ""
done

echo "============================================================"
echo -e "  ${YELLOW}Build summary${NC}  Total time: $(( $(date +%s) - START_ALL ))s"
echo -e "  Success (${#PASSED[@]}): ${GREEN}${PASSED[*]:-(none)}${NC}"
echo -e "  Failed (${#FAILED[@]}): ${RED}${FAILED[*]:-(none)}${NC}"
echo "============================================================"; echo ""
[ ${#FAILED[@]} -gt 0 ] && exit 1; exit 0
