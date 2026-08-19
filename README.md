**简体中文** | [**English**](README-en.md)

# ROS-RTEMS — ROS 2 on RTEMS 6.1 移植与实时执行器

将完整的 **ROS 2 (Humble) 核心中间件栈** 移植到 **RTEMS 6.1** 实时操作系统
（ARM Cortex-A9 / QEMU `realview_pbx_a9` 仿真平台），并在此基础上实现
**RTExecutor**——由 RTEMS 内核原生回调调度驱动的实时执行器，
消除中间件层面的嵌套调度开销。

| | |
|---|---|
| **负责人** | siweimutong |
| **联系方式** | luoxiantong@bjut.edu.cn |
| **开源协议** | Apache-2.0 |

---

## 项目简介

本项目完整移植了 ROS 2（Humble）核心中间件栈：

- **全静态链接**：所有 ROS 2 库编译为 `.a` 静态归档，面向裸机嵌入式部署；
- **硬件定时器驱动**：Cortex-A9 Private Timer 中断驱动（`/dev/rtss_timer`，10 ns 分辨率）；
- **内核级回调调度**：每个 ROS 2 回调映射为独立 RTEMS 实时任务，由内核 FP 抢占调度器管理；
- **消除中间件调度**：RTExecutor 只做就绪检测与事件通知，不做调度决策；
- **进程内零拷贝**：基于 `UniquePtr` 的零拷贝发布/订阅；
- **多策略对比**：6 组实验对比 SingleThreaded / RTExecutor / PiCAS / MultiThreaded 等调度策略。

## 前置环境部署

环境部署有两种途径，详细步骤请参阅：

- **从零搭建 RTEMS 6.1（Ubuntu 22.04）**：见 [RTEMS-DDS-README.md](RTEMS-DDS-README.md)
  —— 使用 RTEMS Source Builder（RSB）构建工具链与
  `arm/realview_pbx_a9_qemu` BSP，并完成 libbsd 与 DDS 各组件的移植编译；
- **RTEMS 6 容器（Phytium D2000 / aarch64）**：见 [RTEMS-README.md](RTEMS-README.md)
  —— 基于 `roker405/rtems6-env:v1.0` 镜像的 `a53_lp64_qemu` BSP 构建与 QEMU 运行；
- **开箱即用的 Docker 开发环境（推荐）**：见 [README-en.md](README-en.md) 第 2 章
  —— `roker405/rtems6-env:v1.0` 镜像已包含完整工具链、BSP 与 QEMU。

## 目录结构

```
├── README.md                 # 本文件（中文简介）
├── README-en.md              # 完整英文文档（构建/运行/排错）
├── RTEMS-README.md           # RTEMS6 (Phytium D2000) 环境部署
├── RTEMS-DDS-README.md       # RTEMS 6.1 quick-start + DDS/ROS2 组件移植
├── build_all.sh              # 统一编译入口脚本
├── build_all_exp*.sh         # 各实验组批量编译脚本
├── run_all*.sh               # 各实验组批量运行脚本
├── run_compare.sh            # 核心调度策略对比脚本
├── RTcolcon                  # 底层库并行编译管理器
│
├── Example/                  # 实验工程（见下）
│   ├── Exp_Intra-MT|ST|Ours|Picas|ROSRT/   # 进程内(intra-process)实验组
│   └── Exp_Inter-MT|ST|RT|Ours|Picas|ROSRT/ # 进程间(inter-process)实验组
│
├── rcutils/ rcl/ rclcpp/ rmw/ rmw_fastrtps/ rosidl/ ...  # ROS 2 底层库源码
├── DDS/                      # Fast-CDR / Fast-DDS 移植
├── rclcpp-rtems/             # 改造版 rclcpp（RTExecutor + DDS 通知）
├── rclcpp-picas/             # 改造版 rclcpp（PiCAS 执行器）
├── rclcpp-rtss25/            # rclcpp-rtss25 基线
├── RTExecutor/ RTROS/ PICAS/ # 执行器/框架独立基准工程
├── intra_process_demo/       # 进程内零拷贝通信演示
├── lifecycle_exp/ pingpong_exp/ examples/ demos/  # 示例与实验包
├── paper/                    # 论文材料
└── Results-*/                # 实验结果输出目录
```

### 实验工程（Example/）

| 工程 | 说明 |
|------|------|
| `Exp_Intra-*` | 进程内（intra-process）调度策略实验组：ST / MT / Ours / Picas / ROSRT |
| `Exp_Inter-*` | 进程间（inter-process）调度策略实验组：ST / MT / RT / Ours / Picas / ROSRT |

每个实验工程内含 `build_all.sh`（编译）、`run.sh`（QEMU 运行）、`wscript`（构建规格）、
`src/`（源码）、`include/`（头文件）、`Result/`（实验结果 CSV）。
各工程 README 均标注了负责人与联系方式。

## 快速开始

```bash
# 1. 部署环境（推荐 Docker 镜像）
docker pull roker405/rtems6-env:v1.0
docker run -d --name rtems6-dev \
  -v ~/docker_ws:/home/siweimutong \
  -w /home/siweimutong/workspace \
  roker405/rtems6-env:v1.0 sleep infinity

# 2. 编译底层库（27 个 ROS 2 核心包，生成 .a 静态库）
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash RTcolcon -j 4 -t 8 force'

# 3. 编译并运行实验（示例：Exp_Intra-MT）
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace/Example/Exp_Intra-MT && ./build_all.sh && ./run.sh all'

# 4. 批量运行全部实验组
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash run_all_exp.sh'
```

更详细的构建、运行、结果解读与排错说明，请阅读 [README-en.md](README-en.md)。

## 许可证

本项目采用 **Apache License 2.0**，详见 [LICENSE](LICENSE)。
仓库中的第三方组件（RTEMS、ROS 2 各包、Fast-DDS 等）保留其各自的开源许可证。
