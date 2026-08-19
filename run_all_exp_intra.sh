#!/bin/bash
#
# run_all_exp_intra.sh — Run sequentially Exp_Intra-ST, Exp_Intra-MT, Exp_Intra-Picas, Exp_Intra-ROSRT, Exp_Intra-Ours
#
# Run from host; launches QEMU inside the rtems6-dev container via docker exec
# Each experiment calls ./run.sh all (n=2,3,4,5); QEMU and CSV output saved in each Result/ dir
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

# ---- Cleanup leftover QEMU processes ----
cleanup_qemu() {
    echo -e "\n${YELLOW}[cleanup] Killing QEMU processes...${NC}"
    docker exec -i "$CONTAINER" bash -lc \
        'pkill -9 qemu-system-arm 2>/dev/null; pkill -9 -f "timeout.*qemu" 2>/dev/null; true' || true
    sleep 0.5
}

# ---- Ctrl+C / signal handler ----
on_interrupt() {
    echo -e "\n${RED}Interrupted by user${NC}"
    cleanup_qemu
    echo -e "${YELLOW}Exiting. Partial results saved in Result/ directories.${NC}"
    exit 130
}
trap on_interrupt INT TERM

# ---- CheckContainer状态 ----
if [ "$(docker inspect -f '{{.State.Running}}' "$CONTAINER" 2>/dev/null)" != "true" ]; then
    echo -e "${RED}Error: Container $CONTAINER is not running${NC}"
    echo "Please start the container first, then run this script."
    exit 1
fi

echo ""
echo "============================================================"
echo -e "  ${CYAN}Exp_Intra Batch Run${NC}"
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
    echo -e "${CYAN}│  Running: ${exp}  (estimated ~10-20 minutes)${NC}"
    echo -e "${CYAN}└──────────────────────────────────────────────────────────┘${NC}"

    EXP_START=$(date +%s)

    # Check if run.sh exists
    if ! docker exec -i "$CONTAINER" bash -lc \
        "test -f ${WORKSPACE}/${exp}/run.sh"; then
        echo -e "  ${RED}Error: ${exp}/run.sh not found${NC}"
        FAILED+=("${exp}")
        continue
    fi

    # Run inside container (QEMUx4 chain lengths, max 600s per chain)
    # timeout --foreground allows SIGINT to propagate to docker exec
    if timeout --foreground 3600 docker exec -i "$CONTAINER" bash -lc \
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

if [ ${#FAILED[@]} -gt 0 ]; then
    exit 1
fi
exit 0
