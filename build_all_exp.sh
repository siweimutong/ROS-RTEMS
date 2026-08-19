#!/bin/bash
#
# build_all_exp.sh — Run all build_all_exp*.sh scripts sequentially
#
# Executes from host, invoking each experiment-family build script in order.
# Each script internally uses docker exec to build inside the rtems6-dev container.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

BUILD_SCRIPTS=(
    "build_all_exp1.sh"
    "build_all_exp_intra.sh"
    "build_all_exp_inter.sh"
    "build_all_exp5.sh"
    "build_all_exp6.sh"
)

echo ""
echo "======================================================================"
echo -e "  ${CYAN}Unified Build — All Experiments${NC}"
echo "  Start time: $(date '+%Y-%m-%d %H:%M:%S')"
echo "  Scripts: ${BUILD_SCRIPTS[*]}"
echo "======================================================================"
echo ""

FAILED=()
PASSED=()
START_ALL=$(date +%s)

for script in "${BUILD_SCRIPTS[@]}"; do
    echo -e "${CYAN}┌────────────────────────────────────────────────────────────────────┐${NC}"
    echo -e "${CYAN}│  Running: ${script}${NC}"
    echo -e "${CYAN}└────────────────────────────────────────────────────────────────────┘${NC}"
    SCRIPT_START=$(date +%s)

    if [ ! -x "$script" ]; then
        echo -e "  ${RED}Error: ${script} not found or not executable${NC}"
        FAILED+=("${script}")
        continue
    fi

    if ./"$script"; then
        SCRIPT_ELAPSED=$(( $(date +%s) - SCRIPT_START ))
        echo -e "  ${GREEN}✓ ${script} succeeded (${SCRIPT_ELAPSED}s)${NC}"
        PASSED+=("${script}")
    else
        SCRIPT_ELAPSED=$(( $(date +%s) - SCRIPT_START ))
        echo -e "  ${RED}✗ ${script} failed (${SCRIPT_ELAPSED}s)${NC}"
        FAILED+=("${script}")
    fi
    echo ""
done

TOTAL_ALL=$(( $(date +%s) - START_ALL ))
echo "======================================================================"
echo -e "  ${YELLOW}Build Summary${NC}"
echo "  Total time: ${TOTAL_ALL}s"
echo -e "  Success (${#PASSED[@]}): ${GREEN}${PASSED[*]:-(none)}${NC}"
echo -e "  Failed  (${#FAILED[@]}): ${RED}${FAILED[*]:-(none)}${NC}"
echo "======================================================================"
echo ""
[ ${#FAILED[@]} -gt 0 ] && exit 1
exit 0
