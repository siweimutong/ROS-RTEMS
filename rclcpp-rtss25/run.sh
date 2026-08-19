#!/bin/bash
# RTcolcon wrapper for rclcpp-rtss25 — builds only core librclcpp.a
# Called by RTcolcon as: ./run.sh <force|no-force> <root_path> [threads]

set -euo pipefail

BUILD_MODE="${1:-no-force}"
ROOT_PATH="${2:-$(pwd)}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONFIG_DIR="${SCRIPT_DIR}/config/config_rclcpp"

cd "$CONFIG_DIR"
./gen_run.sh "$BUILD_MODE" "$ROOT_PATH"
./run.sh "$ROOT_PATH"
