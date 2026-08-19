#!/bin/bash
set -euo pipefail

# =====================================================================
#  build_all.sh — ROS2-RTEMS unified build script
#
#  Manages the build of the low-level libraries and application-layer
#  projects. Low-level libraries are built with RTcolcon, while
#  application-layer projects are managed by this script.
#
#  Usage:
#    ./build_all.sh [options] <build mode>
#
#  Options:
#    -h, --help          Show help message
#    -j, --jobs NUM      RTcolcon maximum number of parallel processes (default: 4)
#    -t, --threads NUM   RTcolcon compile threads per component (default: 8)
#    -l, --libs-only     Build only the low-level libraries
#    -a, --apps-only     Build only the application-layer projects
#    -s, --skip APP      Skip the specified application (can be used multiple times)
#
#  Build modes:
#    force               Force rebuild (clear history markers and rebuild)
#    no-force            Incremental build (build only incomplete components)
#
#  Examples:
#    ./build_all.sh force                        # Build everything (libs + apps)
#    ./build_all.sh -l no-force                  # Incrementally build only the libs
#    ./build_all.sh -a force                     # Force build only the apps
#    ./build_all.sh -s examples force            # Build everything, skip examples
#    ./build_all.sh -j 8 -t 16 force             # Build with 8 processes, 16 threads
# =====================================================================

# ===================== Colors =====================
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

# ===================== Default parameters =====================
DEFAULT_MAX_PROCESSES=4
DEFAULT_COMPILE_THREADS=8
MAX_PROCESSES=$DEFAULT_MAX_PROCESSES
COMPILE_THREADS=$DEFAULT_COMPILE_THREADS
BUILD_LIBS=true
BUILD_APPS=true
SKIP_APPS=()
BUILD_MODE=""

# ===================== Application project definitions =====================
#
# Format: "project_name:config_subdir_path:dependency_list(space-separated)"
#
# Dependency list notes:
#   - "libs" means the low-level libraries must all be built first
#   - Other project names mean the specified application project is a dependency
#
APP_PROJECTS=(
    "intra_process_demo:config:libs"
    "pingpong_exp:config:libs"
    "examples:config:libs"
)

# ===================== Help message =====================
show_help() {
    cat << EOF
ROS2-RTEMS unified build script
Usage: $0 [options] <build mode>

Options:
  -h, --help          Show this help message
  -j, --jobs NUM      RTcolcon maximum number of parallel processes (default: $DEFAULT_MAX_PROCESSES)
  -t, --threads NUM   RTcolcon compile threads per component (default: $DEFAULT_COMPILE_THREADS)
  -l, --libs-only     Build only the low-level libraries (skip apps)
  -a, --apps-only     Build only the application-layer projects (skip libs)
  -s, --skip APP      Skip the specified application project (can be used multiple times)

Build modes:
  force               Force rebuild (clear history markers, rebuild all components)
  no-force            Incremental build (build only incomplete/failed components)

Application projects:
  intra_process_demo   Intra-process communication demo (with hardware timer)
  pingpong_exp         Ping-Pong latency test
  examples             Example programs

Examples:
  $0 force                             # Build everything
  $0 -l no-force                       # Incrementally build only the libs
  $0 -a force                          # Force build only the apps
  $0 -s examples -s exp force          # Skip examples and exp
  $0 -j 8 -t 16 force                  # 8 parallel processes, 16 threads
EOF
    exit 0
}

# ===================== Parse arguments =====================
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            show_help
            ;;
        -j|--jobs)
            if [[ $2 =~ ^[1-9][0-9]*$ ]]; then
                MAX_PROCESSES=$2
                shift 2
            else
                echo -e "${RED}Error: -j/--jobs requires a positive integer${NC}" >&2
                exit 1
            fi
            ;;
        -t|--threads)
            if [[ $2 =~ ^[1-9][0-9]*$ ]]; then
                COMPILE_THREADS=$2
                shift 2
            else
                echo -e "${RED}Error: -t/--threads requires a positive integer${NC}" >&2
                exit 1
            fi
            ;;
        -l|--libs-only)
            BUILD_APPS=false
            shift
            ;;
        -a|--apps-only)
            BUILD_LIBS=false
            shift
            ;;
        -s|--skip)
            if [[ -n "$2" ]]; then
                SKIP_APPS+=("$2")
                shift 2
            else
                echo -e "${RED}Error: -s/--skip requires a project name${NC}" >&2
                exit 1
            fi
            ;;
        force|no-force)
            BUILD_MODE=$1
            shift
            ;;
        *)
            echo -e "${RED}Error: Unknown argument '$1'${NC}" >&2
            show_help
            ;;
    esac
done

if [[ -z "$BUILD_MODE" ]]; then
    echo -e "${RED}Error: a build mode (force/no-force) must be specified${NC}" >&2
    show_help
fi

WORKSPACE_DIR="$(cd "$(dirname "$0")" && pwd)"
START_TIME=$(date +%s)

# ===================== Utility functions =====================

log_info()  { echo -e "[$(date +'%H:%M:%S')] ${BLUE}$1${NC}"; }
log_ok()    { echo -e "[$(date +'%H:%M:%S')] ${GREEN}$1${NC}"; }
log_warn()  { echo -e "[$(date +'%H:%M:%S')] ${YELLOW}$1${NC}"; }
log_fail()  { echo -e "[$(date +'%H:%M:%S')] ${RED}$1${NC}"; }

# Check whether the low-level libraries are built (check if the key .a files
# are installed in the RTEMS system directory)
libs_ready() {
    local lib_dir="$HOME/RTEMS/quick-start/rtems/6/arm-rtems6/realview_pbx_a9_qemu/lib"
    for lib in librclcpp.a librcl.a librmw.a libyaml.a librcutils.a; do
        if [[ ! -f "$lib_dir/$lib" ]]; then
            return 1
        fi
    done
    return 0
}

# Check whether the application is in the skip list
should_skip() {
    local app_name=$1
    for skip in "${SKIP_APPS[@]}"; do
        if [[ "$skip" == "$app_name" ]]; then
            return 0
        fi
    done
    return 1
}

# Parse an application project definition
parse_app() {
    local entry=$1
    local field=$2
    echo "$entry" | cut -d: -f$field
}

# Build a single application project
build_app() {
    local app_name=$1
    local config_subdir=$2
    local deps=$3

    if should_skip "$app_name"; then
        log_warn "Skipping $app_name (--skip)"
        return 0
    fi

    # Check dependencies
    for dep in $deps; do
        if [[ "$dep" == "libs" ]]; then
            if ! libs_ready; then
                log_fail "$app_name: low-level libraries are not fully built, run -l force first"
                return 1
            fi
        else
            if [[ ! -f "$WORKSPACE_DIR/build/.compiled_app_${dep}" ]]; then
                log_fail "$app_name: dependency $dep is not built"
                return 1
            fi
        fi
    done

    local app_dir="$WORKSPACE_DIR/$app_name"
    local config_dir="$app_dir/$config_subdir"

    if [[ ! -d "$app_dir" ]]; then
        log_fail "$app_name: directory does not exist"
        return 1
    fi

    if [[ ! -d "$config_dir" ]]; then
        log_fail "$app_name: config directory does not exist ($config_dir)"
        return 1
    fi

    log_info "Starting build of $app_name"
    local app_start=$(date +%s)

    # Step 1: gen_run.sh (generate wscript and download waf)
    if [[ -f "$config_dir/gen_run.sh" ]]; then
        cd "$config_dir"
        if ! bash gen_run.sh "$BUILD_MODE" "$WORKSPACE_DIR"; then
            log_fail "$app_name: gen_run.sh failed"
            cd "$WORKSPACE_DIR"
            return 1
        fi
        cd "$WORKSPACE_DIR"
    fi

    # Step 2: run.sh (configure + build)
    if [[ -f "$config_dir/run.sh" ]]; then
        cd "$config_dir"
        if ! bash run.sh "$WORKSPACE_DIR"; then
            log_fail "$app_name: run.sh failed"
            cd "$WORKSPACE_DIR"
            return 1
        fi
        cd "$WORKSPACE_DIR"
    fi

    local app_elapsed=$(( $(date +%s) - app_start ))
    log_ok "$app_name build finished (${app_elapsed}s)"

    # Mark the build as complete
    mkdir -p "$WORKSPACE_DIR/build"
    touch "$WORKSPACE_DIR/build/.compiled_app_${app_name}"

    return 0
}

# ===================== Main flow =====================
echo ""
echo "========================================================"
echo "  ROS2-RTEMS unified build"
echo "  Mode: $BUILD_MODE"
echo "  Working directory: $WORKSPACE_DIR"
if [[ "$BUILD_LIBS" == true ]]; then
    echo "  Low-level libraries: build"
else
    echo "  Low-level libraries: skip"
fi
if [[ "$BUILD_APPS" == true ]]; then
    echo "  Application layer: build"
else
    echo "  Application layer: skip"
fi
if [[ ${#SKIP_APPS[@]} -gt 0 ]]; then
    echo "  Skipped applications: ${SKIP_APPS[*]}"
fi
echo "========================================================"
echo ""

FAILED_COUNT=0
SUCCESS_COUNT=0

# ===================== Step 1: Build low-level libraries =====================
if [[ "$BUILD_LIBS" == true ]]; then
    echo ""
    log_info "====== Phase 1: build low-level libraries (RTcolcon) ======"
    echo ""

    if [[ -f "$WORKSPACE_DIR/RTcolcon" ]]; then
        cd "$WORKSPACE_DIR"
        if bash RTcolcon -j "$MAX_PROCESSES" -t "$COMPILE_THREADS" "--$BUILD_MODE"; then
            log_ok "Low-level libraries built"
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            log_fail "Low-level library build failed"
            FAILED_COUNT=$((FAILED_COUNT + 1))
            if [[ "$BUILD_APPS" == true ]]; then
                log_warn "Low-level library build failed; applications may fail to build"
            fi
        fi
    else
        log_fail "RTcolcon script not found"
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi
fi

# ===================== Step 2: Build application projects =====================
if [[ "$BUILD_APPS" == true ]]; then
    echo ""
    log_info "====== Phase 2: build application projects ======"
    echo ""

    if [[ "$BUILD_MODE" == "force" ]]; then
        rm -f "$WORKSPACE_DIR/build/.compiled_app_"*
    fi

    for entry in "${APP_PROJECTS[@]}"; do
        app_name=$(parse_app "$entry" 1)
        config_subdir=$(parse_app "$entry" 2)
        deps=$(parse_app "$entry" 3)

        if build_app "$app_name" "$config_subdir" "$deps"; then
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            FAILED_COUNT=$((FAILED_COUNT + 1))
        fi
    done
fi

# ===================== Summary =====================
END_TIME=$(date +%s)
TOTAL_ELAPSED=$((END_TIME - START_TIME))

echo ""
echo "========================================================"
echo -e "  ${YELLOW}Build summary${NC}"
echo "  Total time: ${TOTAL_ELAPSED}s"
echo -e "  Success: ${GREEN}${SUCCESS_COUNT}${NC}  Failed: ${RED}${FAILED_COUNT}${NC}"
echo "========================================================"
echo ""

if [[ $FAILED_COUNT -gt 0 ]]; then
    exit 1
fi
exit 0
