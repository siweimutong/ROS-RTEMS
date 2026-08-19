#!/bin/bash
#
# build_all_exp_intra.sh — Build sequentially Exp_Intra-ST, Exp_Intra-MT, Exp_Intra-Picas, Exp_Intra-ROSRT, Exp_Intra-Ours
#
# Execute from the host, building inside the rtems6-dev container via docker exec
#
set -euo pipefail

CONTAINER="rtems6-dev"
WORKSPACE="/home/siweimutong/workspace"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

EXPERIMENTS=("Example/Exp_Intra-ST" "Example/Exp_Intra-MT" "Example/Exp_Intra-Picas" "Example/Exp_Intra-ROSRT" "Example/Exp_Intra-Ours")

# ---- Check container status ----
if [ "$(docker inspect -f '{{.State.Running}}' "$CONTAINER" 2>/dev/null)" != "true" ]; then
    echo -e "${RED}Error: Container $CONTAINER is not running${NC}"
    echo "Please start the container first, then run this script."
    exit 1
fi

echo ""
echo "============================================================"
echo -e "  ${CYAN}Exp_Intra Batch Build${NC}"
echo "  Container: $CONTAINER"
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

    # Check if build_all.sh exists
    if ! docker exec -i "$CONTAINER" bash -lc \
        "test -f ${WORKSPACE}/${exp}/build_all.sh"; then
        echo -e "  ${RED}Error: ${exp}/build_all.sh not found${NC}"
        FAILED+=("${exp}")
        continue
    fi

    # Build inside the container
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

if [ ${#FAILED[@]} -gt 0 ]; then
    exit 1
fi
exit 0
