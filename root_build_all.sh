#!/bin/bash
set -euo pipefail

# ===================== Basic configuration =====================
DEFAULT_COMPILE_THREADS=8

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

show_help() {
    cat << EOF
ROS2-RTEMS Build Tool (single-process sequential version)
Usage: $0 [option] <Build mode>

Options:
  -h, --help          Show this help message
  -t, --threads NUM   Threads per component(Default:$DEFAULT_COMPILE_THREADS)

Build modes:
  force               Force rebuild (clear history markers, rebuild all components)
  no-force            Incremental build (only build incomplete/failed components)
EOF
    exit 0
}

root_path=$(pwd)
build_modle=""
COMPILE_THREADS=$DEFAULT_COMPILE_THREADS

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) show_help ;;
        -t|--threads)
            if [[ $2 =~ ^[1-9]+$ ]]; then
                COMPILE_THREADS=$2; shift 2
            else
                echo -e "${RED}Error: -t/--threads requires a positive integer${NC}" >&2; exit 1
            fi
            ;;
        -j|--jobs)
            # Compatible with -j parameter from build_all.sh, ignore it
            shift 2
            ;;
        force|no-force) build_modle=$1; shift ;;
        *) echo -e "${RED}Error: Unknown parameter '$1'${NC}" >&2; show_help ;;
    esac
done

if [[ -z "$build_modle" ]]; then
    echo -e "${RED}Error: must specify a Build mode (force/no-force)${NC}" >&2
    show_help
fi

# ===================== Component definitions =====================
COMPONENTS=(
    "DDS" "yaml" "spdlog" "rcutils" "ament_index_cpp"
    "rcpputils" "class_loader" "rosidl" "logging"
    "rmw" "rmw_dds_common" "rmw_fastrtps" "rmw_implementation" "builtin_interfaces"
    "rcl_interfaces" "composition_interfaces" "unique_identifier_msgs" "action_msgs"
    "example_interfaces" "lifecycle_msgs" "rosgraph_msgs" "statistics_msgs"
    "std_msgs" "rcl" "statistics_collector" "rclcpp" "rclcpp-picas"
)

declare -A DEPENDENCIES
DEPENDENCIES["DDS"]=""
DEPENDENCIES["yaml"]=""
DEPENDENCIES["spdlog"]=""
DEPENDENCIES["rcutils"]=""
DEPENDENCIES["ament_index_cpp"]=""
DEPENDENCIES["rcpputils"]="rcutils"
DEPENDENCIES["logging"]="rcutils rcpputils spdlog"
DEPENDENCIES["class_loader"]="rcutils rcpputils"
DEPENDENCIES["rosidl"]="rcutils rcpputils"
DEPENDENCIES["rmw"]="rcutils rcpputils rosidl"
DEPENDENCIES["rmw_dds_common"]="rcutils rcpputils rosidl rmw"
DEPENDENCIES["rmw_fastrtps"]="DDS rcutils rcpputils rmw rosidl rmw_dds_common"
DEPENDENCIES["rmw_implementation"]="rcutils rcpputils rmw ament_index_cpp"
DEPENDENCIES["builtin_interfaces"]="rcutils DDS rosidl"
DEPENDENCIES["rcl_interfaces"]="DDS rcutils rmw rosidl builtin_interfaces"
DEPENDENCIES["composition_interfaces"]="rcl_interfaces rcutils rosidl DDS rmw"
DEPENDENCIES["unique_identifier_msgs"]="rosidl rcutils DDS"
DEPENDENCIES["action_msgs"]="rosidl unique_identifier_msgs builtin_interfaces rcutils DDS rmw"
DEPENDENCIES["example_interfaces"]="rosidl rcutils unique_identifier_msgs builtin_interfaces action_msgs rmw DDS"
DEPENDENCIES["lifecycle_msgs"]="rosidl rcutils DDS rmw"
DEPENDENCIES["rosgraph_msgs"]="rosidl rcutils builtin_interfaces DDS"
DEPENDENCIES["rcl"]="rmw rcutils rosidl rmw_implementation rcl_interfaces logging"
DEPENDENCIES["statistics_msgs"]="rosidl rcutils builtin_interfaces DDS"
DEPENDENCIES["statistics_collector"]="rcpputils rosidl builtin_interfaces rcl rmw statistics_msgs"
DEPENDENCIES["std_msgs"]="rosidl rcutils DDS builtin_interfaces"
DEPENDENCIES["rclcpp"]="rmw rcpputils rcutils rcl builtin_interfaces rcl_interfaces rosgraph_msgs statistics_msgs statistics_collector ament_index_cpp rosidl"
DEPENDENCIES["rclcpp-picas"]="rmw rcpputils rcutils rcl builtin_interfaces rcl_interfaces rosgraph_msgs statistics_msgs statistics_collector ament_index_cpp rosidl"

# ===================== Single-process sequential build =====================
COMPILED_SET=()
FAILED_SET=()

is_compiled() {
    local comp=$1
    for c in "${COMPILED_SET[@]}"; do
        [[ "$c" == "$comp" ]] && return 0
    done
    return 1
}

is_failed() {
    local comp=$1
    for c in "${FAILED_SET[@]}"; do
        [[ "$c" == "$comp" ]] && return 0
    done
    return 1
}

compile_one() {
    local component=$1

    # Skip if already built
    if is_compiled "$component"; then
        return 0
    fi

    # Build dependencies first
    for dep in ${DEPENDENCIES[$component]}; do
        if ! is_compiled "$dep"; then
            compile_one "$dep" || {
                echo -e "[$(date +'%H:%M:%S')] ${RED}$component: dependency $dep Build failed,Skipping${NC}"
                FAILED_SET+=("$component")
                return 1
            }
        fi
    done

    echo -e "[$(date +'%H:%M:%S')] ${BLUE}Starting >>> $component${NC}"
    local start_time=$(date +%s)

    cd "$root_path"
    if [[ ! -d "$component" ]]; then
        echo -e "[$(date +'%H:%M:%S')] ${RED}$component: directorynot found${NC}"
        FAILED_SET+=("$component")
        return 1
    fi

    cd "$component"
    local res=0
    if [[ -f "./run.sh" ]]; then
        ./run.sh "$build_modle" "$root_path" "$COMPILE_THREADS" || res=1
    else
        mkdir -p build && cd build
        cmake .. || res=1
        if [[ $res -eq 0 ]]; then
            cmake --build . -j${COMPILE_THREADS} || res=1
        fi
    fi
    cd "$root_path"

    local elapsed=$(( $(date +%s) - start_time ))
    if [[ $res -eq 0 ]]; then
        COMPILED_SET+=("$component")
        echo -e "[$(date +'%H:%M:%S')] ${GREEN}Finished <<< $component [${elapsed}s]${NC}"
    else
        FAILED_SET+=("$component")
        echo -e "[$(date +'%H:%M:%S')] ${RED}Failed <<< $component [${elapsed}s]${NC}"
    fi

    return $res
}

# ===================== Main flow =====================
echo ""
echo "========================================================"
echo "  ROS2-RTEMS Build (single-process sequential mode)"
echo "  Mode: $build_modle"
echo "  Working directory: $root_path"
echo "  Thread count: $COMPILE_THREADS"
echo "  Component count: ${#COMPONENTS[@]}"
echo "========================================================"
echo ""

START_TIME=$(date +%s)

for comp in "${COMPONENTS[@]}"; do
    compile_one "$comp" || true
done

END_TIME=$(date +%s)
TOTAL_ELAPSED=$((END_TIME - START_TIME))

echo ""
echo "========================================================"
echo -e "  ${YELLOW}Build summary${NC}"
echo "  Total time: ${TOTAL_ELAPSED}s"
echo -e "  Success: ${GREEN}${#COMPILED_SET[@]}${NC}  Failed: ${RED}${#FAILED_SET[@]}${NC}"
if [[ ${#FAILED_SET[@]} -gt 0 ]]; then
    echo -e "  Failed components: ${RED}${FAILED_SET[*]}${NC}"
fi
echo "========================================================"
echo ""

if [[ ${#FAILED_SET[@]} -gt 0 ]]; then
    exit 1
fi
exit 0
