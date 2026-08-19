#!/bin/bash
set -euo pipefail

# =====================================================================
#  build_all.sh — ROS2-RTEMS 统一编译脚本
#
#  统一管理底层库和应用层项目的编译。底层库使用 RTcolcon 编译，
#  应用层项目使用本脚本管理。
#
#  用法：
#    ./build_all.sh [选项] <编译模式>
#
#  选项：
#    -h, --help          显示帮助信息
#    -j, --jobs NUM      RTcolcon 最大并行数（默认: 4）
#    -t, --threads NUM   RTcolcon 单组件编译线程数（默认: 8）
#    -l, --libs-only     仅编译底层库
#    -a, --apps-only     仅编译应用层项目
#    -s, --skip APP      跳过指定应用（可多次使用）
#
#  编译模式：
#    force               强制编译（清空历史标记，重新编译）
#    no-force            增量编译（仅编译未完成的组件）
#
#  示例：
#    ./build_all.sh force                        # 编译全部（库+应用）
#    ./build_all.sh -l no-force                  # 仅增量编译库
#    ./build_all.sh -a force                     # 仅强制编译应用
#    ./build_all.sh -s examples force            # 编译全部，跳过 examples
#    ./build_all.sh -j 8 -t 16 force             # 8并行、16线程编译
# =====================================================================

# ===================== 颜色 =====================
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

# ===================== 默认参数 =====================
DEFAULT_MAX_PROCESSES=4
DEFAULT_COMPILE_THREADS=8
MAX_PROCESSES=$DEFAULT_MAX_PROCESSES
COMPILE_THREADS=$DEFAULT_COMPILE_THREADS
BUILD_LIBS=true
BUILD_APPS=true
SKIP_APPS=()
BUILD_MODE=""

# ===================== 应用层项目定义 =====================
#
# 格式："项目名:config子目录路径:依赖列表(空格分隔)"
#
# 依赖列表说明：
#   - "libs" 表示依赖底层库全部编译完成
#   - 其他项目名表示依赖特定应用项目
#
APP_PROJECTS=(
    "intra_process_demo:config:libs"
    "pingpong_exp:config:libs"
    "examples:config:libs"
)

# ===================== 帮助信息 =====================
show_help() {
    cat << EOF
ROS2-RTEMS 统一编译脚本
用法：$0 [选项] <编译模式>

选项：
  -h, --help          显示此帮助信息
  -j, --jobs NUM      RTcolcon 最大并行数（默认：$DEFAULT_MAX_PROCESSES）
  -t, --threads NUM   RTcolcom 单组件编译线程数（默认：$DEFAULT_COMPILE_THREADS）
  -l, --libs-only     仅编译底层库（跳过应用层）
  -a, --apps-only     仅编译应用层项目（跳过底层库）
  -s, --skip APP      跳过指定应用项目（可多次使用）

编译模式：
  force               强制编译（清空历史标记，重新编译所有组件）
  no-force            增量编译（仅编译未完成/失败的组件）

应用层项目：
  intra_process_demo   进程内通信演示（含硬件定时器）
  pingpong_exp         Ping-Pong 延迟测试
  examples             示例程序

示例：
  $0 force                             # 编译全部
  $0 -l no-force                       # 仅增量编译底层库
  $0 -a force                          # 仅强制编译应用层
  $0 -s examples -s exp force          # 跳过 examples 和 exp
  $0 -j 8 -t 16 force                  # 8并行、16线程
EOF
    exit 0
}

# ===================== 解析参数 =====================
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
                echo -e "${RED}错误：-j/--jobs 后必须跟正整数${NC}" >&2
                exit 1
            fi
            ;;
        -t|--threads)
            if [[ $2 =~ ^[1-9][0-9]*$ ]]; then
                COMPILE_THREADS=$2
                shift 2
            else
                echo -e "${RED}错误：-t/--threads 后必须跟正整数${NC}" >&2
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
                echo -e "${RED}错误：-s/--skip 后必须跟项目名${NC}" >&2
                exit 1
            fi
            ;;
        force|no-force)
            BUILD_MODE=$1
            shift
            ;;
        *)
            echo -e "${RED}错误：未知参数 '$1'${NC}" >&2
            show_help
            ;;
    esac
done

if [[ -z "$BUILD_MODE" ]]; then
    echo -e "${RED}错误：必须指定编译模式（force/no-force）${NC}" >&2
    show_help
fi

WORKSPACE_DIR="$(cd "$(dirname "$0")" && pwd)"
START_TIME=$(date +%s)

# ===================== 工具函数 =====================

log_info()  { echo -e "[$(date +'%H:%M:%S')] ${BLUE}$1${NC}"; }
log_ok()    { echo -e "[$(date +'%H:%M:%S')] ${GREEN}$1${NC}"; }
log_warn()  { echo -e "[$(date +'%H:%M:%S')] ${YELLOW}$1${NC}"; }
log_fail()  { echo -e "[$(date +'%H:%M:%S')] ${RED}$1${NC}"; }

# 检查底层库是否已编译（检查关键 .a 文件是否安装到 RTEMS 系统目录）
libs_ready() {
    local lib_dir="$HOME/RTEMS/quick-start/rtems/6/arm-rtems6/realview_pbx_a9_qemu/lib"
    for lib in librclcpp.a librcl.a librmw.a libyaml.a librcutils.a; do
        if [[ ! -f "$lib_dir/$lib" ]]; then
            return 1
        fi
    done
    return 0
}

# 检查应用是否在跳过列表中
should_skip() {
    local app_name=$1
    for skip in "${SKIP_APPS[@]}"; do
        if [[ "$skip" == "$app_name" ]]; then
            return 0
        fi
    done
    return 1
}

# 解析应用项目定义
parse_app() {
    local entry=$1
    local field=$2
    echo "$entry" | cut -d: -f$field
}

# 编译单个应用项目
build_app() {
    local app_name=$1
    local config_subdir=$2
    local deps=$3

    if should_skip "$app_name"; then
        log_warn "跳过 $app_name（--skip）"
        return 0
    fi

    # 检查依赖
    for dep in $deps; do
        if [[ "$dep" == "libs" ]]; then
            if ! libs_ready; then
                log_fail "$app_name：底层库未编译完成，请先运行 -l force"
                return 1
            fi
        else
            if [[ ! -f "$WORKSPACE_DIR/build/.compiled_app_${dep}" ]]; then
                log_fail "$app_name：依赖 $dep 未编译"
                return 1
            fi
        fi
    done

    local app_dir="$WORKSPACE_DIR/$app_name"
    local config_dir="$app_dir/$config_subdir"

    if [[ ! -d "$app_dir" ]]; then
        log_fail "$app_name：目录不存在"
        return 1
    fi

    if [[ ! -d "$config_dir" ]]; then
        log_fail "$app_name：config 目录不存在 ($config_dir)"
        return 1
    fi

    log_info "开始编译 $app_name"
    local app_start=$(date +%s)

    # Step 1: gen_run.sh（生成 wscript 和下载 waf）
    if [[ -f "$config_dir/gen_run.sh" ]]; then
        cd "$config_dir"
        if ! bash gen_run.sh "$BUILD_MODE" "$WORKSPACE_DIR"; then
            log_fail "$app_name：gen_run.sh 失败"
            cd "$WORKSPACE_DIR"
            return 1
        fi
        cd "$WORKSPACE_DIR"
    fi

    # Step 2: run.sh（configure + build）
    if [[ -f "$config_dir/run.sh" ]]; then
        cd "$config_dir"
        if ! bash run.sh "$WORKSPACE_DIR"; then
            log_fail "$app_name：run.sh 失败"
            cd "$WORKSPACE_DIR"
            return 1
        fi
        cd "$WORKSPACE_DIR"
    fi

    local app_elapsed=$(( $(date +%s) - app_start ))
    log_ok "$app_name 编译完成 (${app_elapsed}s)"

    # 标记编译完成
    mkdir -p "$WORKSPACE_DIR/build"
    touch "$WORKSPACE_DIR/build/.compiled_app_${app_name}"

    return 0
}

# ===================== 主流程 =====================
echo ""
echo "========================================================"
echo "  ROS2-RTEMS 统一编译"
echo "  模式: $BUILD_MODE"
echo "  工作目录: $WORKSPACE_DIR"
if [[ "$BUILD_LIBS" == true ]]; then
    echo "  底层库: 编译"
else
    echo "  底层库: 跳过"
fi
if [[ "$BUILD_APPS" == true ]]; then
    echo "  应用层: 编译"
else
    echo "  应用层: 跳过"
fi
if [[ ${#SKIP_APPS[@]} -gt 0 ]]; then
    echo "  跳过应用: ${SKIP_APPS[*]}"
fi
echo "========================================================"
echo ""

FAILED_COUNT=0
SUCCESS_COUNT=0

# ===================== Step 1: 编译底层库 =====================
if [[ "$BUILD_LIBS" == true ]]; then
    echo ""
    log_info "====== 阶段 1：编译底层库 (RTcolcon) ======"
    echo ""

    if [[ -f "$WORKSPACE_DIR/RTcolcon" ]]; then
        cd "$WORKSPACE_DIR"
        if bash RTcolcon -j "$MAX_PROCESSES" -t "$COMPILE_THREADS" "--$BUILD_MODE"; then
            log_ok "底层库编译完成"
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            log_fail "底层库编译失败"
            FAILED_COUNT=$((FAILED_COUNT + 1))
            if [[ "$BUILD_APPS" == true ]]; then
                log_warn "底层库编译失败，应用层可能无法编译"
            fi
        fi
    else
        log_fail "未找到 RTcolcon 脚本"
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi
fi

# ===================== Step 2: 编译应用层项目 =====================
if [[ "$BUILD_APPS" == true ]]; then
    echo ""
    log_info "====== 阶段 2：编译应用层项目 ======"
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

# ===================== 总结 =====================
END_TIME=$(date +%s)
TOTAL_ELAPSED=$((END_TIME - START_TIME))

echo ""
echo "========================================================"
echo -e "  ${YELLOW}编译总结${NC}"
echo "  总耗时: ${TOTAL_ELAPSED}s"
echo -e "  成功: ${GREEN}${SUCCESS_COUNT}${NC}  失败: ${RED}${FAILED_COUNT}${NC}"
echo "========================================================"
echo ""

if [[ $FAILED_COUNT -gt 0 ]]; then
    exit 1
fi
exit 0
