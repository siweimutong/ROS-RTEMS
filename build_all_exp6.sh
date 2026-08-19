#!/bin/bash
#
# build_all_exp6.sh — Build Exp6-Ours, Exp6-ROSRT
#
set -euo pipefail
CONTAINER="rtems6-dev"; WORKSPACE="/home/siweimutong/workspace"
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'
EXPERIMENTS=("Example/Exp6-Ours" "Example/Exp6-ROSRT")

[ "$(docker inspect -f '{{.State.Running}}' "$CONTAINER" 2>/dev/null)" = "true" ] || { echo -e "${RED}Container $CONTAINER is not running${NC}"; exit 1; }

echo ""; echo "============================================================"
echo -e "  ${CYAN}Exp6 Batch Build${NC}"; echo "  Start: $(date '+%Y-%m-%d %H:%M:%S')"
echo "============================================================"; echo ""
FAILED=(); PASSED=(); START_ALL=$(date +%s)

for exp in "${EXPERIMENTS[@]}"; do
    echo -e "${CYAN}┌──────────────────────────────────────────────────────────┐${NC}"
    echo -e "${CYAN}│  Building: ${exp}${NC}"
    echo -e "${CYAN}└──────────────────────────────────────────────────────────┘${NC}"
    EXP_START=$(date +%s)
    if docker exec -i "$CONTAINER" bash -lc "test -f ${WORKSPACE}/${exp}/build_all.sh" && \
       docker exec -i "$CONTAINER" bash -lc "cd ${WORKSPACE}/${exp} && ./build_all.sh"; then
        echo -e "  ${GREEN}✓ ${exp} ($(( $(date +%s) - EXP_START ))s)${NC}"; PASSED+=("${exp}")
    else
        echo -e "  ${RED}✗ ${exp} ($(( $(date +%s) - EXP_START ))s)${NC}"; FAILED+=("${exp}")
    fi; echo ""
done

echo "============================================================"
echo -e "  ${YELLOW}Build summary${NC}  Total time: $(( $(date +%s) - START_ALL ))s"
echo -e "  Success: ${GREEN}${PASSED[*]:-(none)}${NC}  Failed: ${RED}${FAILED[*]:-(none)}${NC}"
echo "============================================================"; echo ""
[ ${#FAILED[@]} -gt 0 ] && exit 1; exit 0
