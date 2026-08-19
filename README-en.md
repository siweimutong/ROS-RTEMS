[**简体中文**](README.md) | **English**

# ROS 2 on RTEMS 6.1 — Porting & Real-Time Executor

A full ROS 2 (Humble) middleware stack ported to RTEMS 6.1 RTOS (ARM realview_pbx_a9_qemu), with RTExecutor eliminating nested scheduling via OS-native callback dispatch.

| | |
|---|---|
| **Owner (负责人)** | siweimutong |
| **Contact (联系方式)** | luoxiantong@bjut.edu.cn |
| **License** | Apache-2.0 |

---

## Table of Contents

- [1. Project Overview](#1-project-overview)
- [2. Prerequisite Environment Deployment (前置环境部署)](#2-prerequisite-environment-deployment)
  - [2.1 RTEMS Quick-Start (From Scratch, Ubuntu 22.04)](#21-rtems-quick-start-from-scratch-ubuntu-2204)
  - [2.2 RTEMS 6 for Phytium D2000 (aarch64)](#22-rtems-6-for-phytium-d2000-aarch64)
  - [2.3 Prerequisites](#23-prerequisites)
  - [2.4 Obtain the Docker Image](#24-obtain-the-docker-image)
    - [Option A: Pull directly (with network)](#option-a-pull-directly-with-network)
    - [Option B: Offline import (cross-machine deployment)](#option-b-offline-import-cross-machine-deployment)
  - [2.5 Create and Start the Container](#25-create-and-start-the-container)
  - [2.6 Using the Container](#26-using-the-container)
  - [2.7 Verify the Container Environment](#27-verify-the-container-environment)
  - [2.8 Container Environment Reference](#28-container-environment-reference)
- [3. Build Process](#3-build-process)
  - [3.1 Build System Overview](#31-build-system-overview)
  - [3.2 Base Library Build (RTcolcon)](#32-base-library-build-rtcolcon)
  - [3.3 Application-Level Build](#33-application-level-build)
  - [3.4 Unified Build Script — build_all.sh](#34-unified-build-script--build_allsh)
  - [3.5 Individual Experiment Group Build](#35-individual-experiment-group-build)
- [4. Run Process](#4-run-process)
  - [4.1 Running with QEMU](#41-running-with-qemu)
  - [4.2 Batch Experiment Execution](#42-batch-experiment-execution)
  - [4.3 Interpreting Results](#43-interpreting-results)
- [5. Project Directory Structure](#5-project-directory-structure)
- [6. Command Quick Reference](#6-command-quick-reference)
- [7. Troubleshooting](#7-troubleshooting)

---

## 1. Project Overview

This project ports the complete ROS 2 (Humble) core middleware stack to RTEMS 6.1, running on an ARM Cortex-A9 platform (QEMU realview_pbx_a9 emulation).

### Key Features

| Feature | Description |
|---------|-------------|
| **Fully static linking** | All ROS 2 libraries compiled as `.a` static archives for bare-metal embedded deployment |
| **Hardware timer driver** | Cortex-A9 Private Timer ISR-driven (`/dev/rtss_timer`), 10 ns resolution |
| **Kernel-level callback scheduling** | Each ROS 2 callback mapped to an independent RTEMS real-time task, managed by the kernel FP preemptive scheduler |
| **Middleware scheduling eliminated** | RTExecutor only handles readiness detection and event notification — no scheduling decisions |
| **Intra-process zero-copy** | `UniquePtr`-based zero-copy publish/subscribe via intra-process communication |
| **Multi-strategy comparison** | 6 experiment groups comparing SingleThreaded / RTExecutor / PiCAS / MultiThreaded strategies |

### Technical Specifications

| Item | Detail |
|------|--------|
| RTOS | RTEMS 6.1 |
| BSP | arm/realview_pbx_a9_qemu |
| Compiler | arm-rtems6-gcc (GCC 12.x) |
| Build system | Waf 2.0.19 + rtems_waf |
| Target platform | QEMU ARM (realview-pbx-a9) |
| Hardware timer | Cortex-A9 Private Timer (0x1f000600, IRQ 29, 100 MHz) |
| System tick | 1 ms (Global Timer, IRQ 27) |
| Base library count | 27 ROS 2 core packages |

---

## 2. Prerequisite Environment Deployment (前置环境部署)
### Overview

The prerequisite RTEMS environment can be deployed in three ways. Full
step-by-step documents are provided in:
- [RTEMS-DDS-README.md](RTEMS-DDS-README.md) — RTEMS 6.1 quick-start from
  scratch (Ubuntu 22.04, `arm/realview_pbx_a9_qemu` BSP) plus DDS / ROS 2
  component porting notes;
- [RTEMS-README.md](RTEMS-README.md) — RTEMS 6 container build for Phytium
  D2000 (`a53_lp64_qemu` BSP) and QEMU execution.

### 2.1 RTEMS Quick-Start (From Scratch, Ubuntu 22.04)

Build the RTEMS 6.1 toolchain and BSP from source (condensed from
[RTEMS-DDS-README.md](RTEMS-DDS-README.md)):

```bash
# step 1: fetch the RTEMS Source Builder (RSB) 6.1
mkdir -p $HOME/RTEMS/quick-start/src && cd $HOME/RTEMS/quick-start/src
curl https://ftp.rtems.org/pub/rtems/releases/6/6.1/sources/rtems-source-builder-6.1.tar.xz | tar xJf -
mv rtems-source-builder-6.1 rsb

# step 2: install host dependencies
sudo apt install -y bison flex build-essential

# step 3: build the ARM toolchain
cd $HOME/RTEMS/quick-start/src/rsb/rtems
../source-builder/sb-set-builder --prefix=$HOME/RTEMS/quick-start/rtems/6 6/rtems-arm

# step 4: verify the toolchain
$HOME/RTEMS/quick-start/rtems/6/bin/arm-rtems6-gcc --version

# step 5: build the BSP (realview_pbx_a9_qemu, with tests)
../source-builder/sb-set-builder --prefix=$HOME/RTEMS/quick-start/rtems/6 \
  --target=arm-rtems6 \
  --with-rtems-bsp=arm/realview_pbx_a9_qemu --with-rtems-tests=yes \
  6/rtems-kernel

# step 6: build libbsd (needed by the network stack)
cd $HOME/RTEMS/quick-start/src/bsd   # rtems-libbsd-6.1 unpacked here
./waf configure --rtems=$HOME/RTEMS/quick-start/rtems/6 \
  --prefix=$HOME/RTEMS/quick-start/rtems/6 \
  --rtems-bsp=arm/realview_pbx_a9_qemu
./waf build && ./waf install
```

A "hello world" example and the full DDS / ROS 2 component porting procedure
(tinyxml2, memory-main, Fast-CDR, Fast-DDS, and the 27 ROS 2 packages, built
with `RTcolcon`) are described in detail in
[RTEMS-DDS-README.md](RTEMS-DDS-README.md).

### 2.2 RTEMS 6 for Phytium D2000 (aarch64)

A ready-made RTEMS 6 development container targeting the Phytium D2000
(`aarch64/a53_lp64_qemu` BSP, condensed from
[RTEMS-README.md](RTEMS-README.md)):

```bash
# pull the RTEMS6 development environment and start the container
docker pull roker405/rtems6-env:v1.0
docker run -it --name rtems6-dev --net=host roker405/rtems6-env:v1.0 bash
docker exec -it -e "TERM=xterm-256color" rtems6-dev bash

# fetch the Phytium-version RTEMS source and configure the BSP
cd /usr/src
git clone https://github.com/The-Lyc/RTContainer.git rtems6
cd rtems6
cat << EOF > config.ini
[DEFAULT]
BUILD_TESTS = True
RTEMS_POSIX_API = True

[aarch64/a53_lp64_qemu]
EOF

# build and install the BSP
./waf configure --prefix=/opt/rtems6
./waf build
./waf install

# run the hello world sample under QEMU
apt install qemu-system-aarch64
qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a53 -smp 1 -m 512M \
  -nographic -no-reboot \
  -kernel build/aarch64/a53_lp64_qemu/testsuites/samples/hello.exe
```


### 2.3 Prerequisites

**Host machine requirements:**

| Item | Requirement |
|------|-------------|
| Operating System | Ubuntu 20.04 / 22.04 LTS, or any Linux distribution with Docker support |
| Docker | >= 20.10 |
| Disk space | >= 15 GB (image ~8 GB + build artifacts) |
| Memory | >= 8 GB (16 GB recommended for parallel builds) |

**Install Docker (if not already installed):**

```bash
# Ubuntu
sudo apt update
sudo apt install -y docker.io
sudo systemctl enable --now docker

# Add current user to docker group (to avoid sudo)
sudo usermod -aG docker $USER
# Log out and back in for the change to take effect
```

### 2.4 Obtain the Docker Image

#### Option A: Pull directly (with network)

```bash
docker pull roker405/rtems6-env:v1.0
```

#### Option B: Offline import (cross-machine deployment)

If the image has been exported as a tar file, import it on the target machine:

```bash
# Import the image
docker load -i rtems6-env-v1.0.tar

# Verify the import
docker image ls roker405/rtems6-env:v1.0
```

**Appendix: How to export the image from the source machine (for reference):**

```bash
# On a machine that already has the image
docker save -o rtems6-env-v1.0.tar roker405/rtems6-env:v1.0

# Compressed export (saves space, ~7.9 GB → ~2.2 GB)
docker save roker405/rtems6-env:v1.0 | gzip > rtems6-env-v1.0.tar.gz

# Decompress and import
gunzip -c rtems6-env-v1.0.tar.gz | docker load
```

The image is approximately 7.9 GB and includes:
- Ubuntu 20.04.6 LTS base system
- RTEMS 6.1 cross-compilation toolchain (`arm-rtems6-gcc` and related tools)
- RTEMS 6.1 BSP (`arm/realview_pbx_a9_qemu`)
- QEMU ARM emulator (v4.2.1)
- Waf build system
- Python 3 + required dependencies

### 2.5 Create and Start the Container

**First-time container creation:**

```bash
docker run -d --name rtems6-dev \
  -v ~/docker_ws:/home/siweimutong \
  -w /home/siweimutong/workspace \
  roker405/rtems6-env:v1.0 \
  sleep infinity
```

Parameter reference:

| Parameter | Description |
|-----------|-------------|
| `-d` | Run in background (keeps container alive for `docker exec`) |
| `--name rtems6-dev` | Container name (must match the scripts) |
| `-v ~/docker_ws:/home/siweimutong` | Mount host project directory into the container's user home |
| `-w /home/siweimutong/workspace` | Set working directory inside the container |
| `sleep infinity` | Keep the container running (lightweight, zero CPU usage) |

**Path mapping:**

```
Host: ~/docker_ws/workspace/    ↔  Container: /home/siweimutong/workspace/
Host: ~/docker_ws/              ↔  Container: /home/siweimutong/
```

> **Note**: The container does not need to run permanently. Stop it when not in use and start it again when needed:
>
> ```bash
> docker stop rtems6-dev      # Stop after finishing work
> docker start rtems6-dev     # Start again when needed
> ```

### 2.6 Using the Container

Once the container is running, execute commands inside it via `docker exec`. All build and run operations use this pattern:

```bash
# Basic pattern
docker exec -i rtems6-dev bash -lc '<command>'

# Example: list workspace contents
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && pwd && ls'
```

If the container is not running, start it first:

```bash
docker start rtems6-dev && \
  docker exec -i rtems6-dev bash -lc '<command>'
```

**Stop the container when done (optional):**

```bash
docker stop rtems6-dev
```

### 2.7 Verify the Container Environment

```bash
# Check if container is running
docker inspect -f '{{.State.Running}}' rtems6-dev
# Expected output: true

# List RTEMS toolchain
docker exec -i rtems6-dev bash -lc \
  'ls /opt/rtems6/bin/ | head -10'

# Check RTEMS BSP
docker exec -i rtems6-dev bash -lc \
  'ls $HOME/RTEMS/quick-start/rtems/6/'

# Check workspace files
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && pwd && ls'

# Check QEMU version
docker exec -i rtems6-dev bash -lc \
  'qemu-system-arm --version'
```

If you need to enter an interactive shell inside the container (for debugging or manual operations):

```bash
docker exec -it rtems6-dev bash
```

### 2.8 Container Environment Reference

| Path | Description |
|------|-------------|
| `/opt/rtems6/bin/` | RTEMS 6 cross-compilation toolchain |
| `$HOME/RTEMS/quick-start/rtems/6/` | RTEMS 6 installation directory (with BSP) |
| `$HOME/RTEMS/quick-start/rtems/6/arm-rtems6/realview_pbx_a9_qemu/lib/` | Build artifact `.a` installation directory |
| `$HOME/RTEMS/quick-start/src/` | RTEMS kernel and RSB source code |
| `/home/siweimutong/workspace/` | Project workspace (mount point) |

---

## 3. Build Process

### 3.1 Build System Overview

```
                    build_all.sh (unified entry point)
                       /          \
                      /            \
           RTcolcon (base libs)    Application-level build
           bash RTcolcon           gen_run.sh → run.sh
           force/no-force          (Waf configure + build)
                │                      │
                ▼                      ▼
          27 ROS 2 packages       Experiments / Demos / Examples
          compiled as .a          compiled as .exe (ELF)
                │                      │
                └──────────┬───────────┘
                           ▼
         Installed to BSP lib/ directory, loaded by QEMU
```

**Two build target categories:**

| Category | Tool | Artifact | Output Location |
|----------|------|----------|-----------------|
| Base libraries (27) | RTcolcon | `lib*.a` static archives | `$HOME/RTEMS/quick-start/rtems/6/arm-rtems6/realview_pbx_a9_qemu/lib/` |
| Application projects | Waf (`gen_run.sh` + `run.sh`) | `*.exe` executables | `<project>/tmp/build/` |

### 3.2 Base Library Build (RTcolcon)

RTcolcon is the project's parallel build manager, handling dependency topology resolution and incremental builds across 27 ROS 2 core packages.

**Usage:**

```bash
# Inside container
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash RTcolcon --help'
```

| Argument | Description | Default |
|----------|-------------|---------|
| `force` | Clean build (clear markers, rebuild all) | — |
| `no-force` | Incremental build (only incomplete components) | — |
| `-j, --jobs N` | Max parallel build jobs | 4 |
| `-t, --threads N` | Compile threads per component | 8 |

**Typical usage:**

```bash
# First time: force rebuild all base libraries
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash RTcolcon -j 4 -t 8 force'

# Subsequent incremental builds (only failed/incomplete libs)
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash RTcolcon -j 4 -t 8 no-force'

# High-performance machine: increase parallelism
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash RTcolcon -j 8 -t 16 force'
```

**Base library dependency topology (build order resolved automatically by RTcolcon):**

```
DDS → yaml → spdlog → rcutils → ament_index_cpp → rcpputils
  │                                                      │
  └→ rosidl → class_loader → rmw → rmw_dds_common        │
                                    │                    │
                                    └→ rmw_fastrtps → rmw_implementation
                                                                          │
  builtin_interfaces → rcl_interfaces → composition_interfaces            │
  lifecycle_msgs → rosgraph_msgs → statistics_msgs → std_msgs             │
                               logging → rcl → statistics_collector → rclcpp
```

**Verify build artifacts:**

```bash
docker exec -i rtems6-dev bash -lc \
  'ls $HOME/RTEMS/quick-start/rtems/6/arm-rtems6/realview_pbx_a9_qemu/lib/*.a | wc -l'
# Expected: 50+ .a files
```

Key static libraries:

| Library File | Component |
|-------------|-----------|
| `librclcpp.a` | ROS 2 C++ Client Library |
| `librcl.a` | ROS 2 C Client Library |
| `librmw.a` | ROS Middleware Abstraction |
| `librmw_fastrtps_cpp.a` | Fast-DDS Adaptation Layer |
| `libfastrtps.a` | Fast-DDS Implementation |
| `libyaml.a` | YAML Parsing |
| `librcutils.a` | C Utility Library |
| `librcpputils.a` | C++ Utility Classes |
| `libspdlog.a` | High-Performance Logging |

### 3.3 Application-Level Build

Each application project follows a uniform two-phase build process:

```
 gen_run.sh                           run.sh
 ──────────                          ────────
 ① Create tmp/ directory      →     ① waf configure (specify RTEMS BSP)
 ② Download waf-2.0.19              ② waf build
 ③ Initialize rtems_waf submodule   ③ Generate .exe artifact
 ④ Generate init.c (RTEMS config)
 ⑤ Generate wscript (build spec)
```

**Example: building `intra_process_demo`:**

```bash
# Enter the project directory
cd /home/siweimutong/workspace/intra_process_demo

# Phase 1: Generate Waf build environment
bash config/gen_run.sh

# Phase 2: Configure and compile
bash config/run.sh
```

What `config/run.sh` does internally:

```bash
cd tmp
rm -rf ./.waf* ./build
./waf configure \
  --rtems=$HOME/RTEMS/quick-start/rtems/6 \
  --rtems-bsp=arm/realview_pbx_a9_qemu
./waf build
```

Build artifact: `tmp/build/<target>.exe`

### 3.4 Unified Build Script — build_all.sh

`build_all.sh` is the unified build entry point, managing both base libraries and all application-level projects.

**Usage:**

```bash
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash build_all.sh --help'
```

**Common scenarios:**

```bash
# Scenario 1: First-time full build (libraries + all apps)
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash build_all.sh force'

# Scenario 2: Incremental build of base libraries only
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash build_all.sh -l no-force'

# Scenario 3: Build applications only
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash build_all.sh -a force'

# Scenario 4: Build everything, skip specific projects
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash build_all.sh -s examples -s exp force'

# Scenario 5: High-parallelism build
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash build_all.sh -j 8 -t 16 force'
```

**Application-level projects and their dependencies:**

| Project | Config Subdirectory | Depends On | Description |
|---------|---------------------|------------|-------------|
| `intra_process_demo` | `config` | libs | Intra-process communication demo (with hardware timer) |
| `pingpong_exp` | `config` | libs | Ping-pong latency test |
| `examples` | `config` | libs | Basic ROS 2 examples |

### 3.5 Individual Experiment Group Build

The following experiment groups have independent build scripts and can be compiled individually:

```bash
# Example: Build Exp_Inter-RT experiment group
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace/Example/Exp_Inter-RT && ./build_all.sh'
```

BSP build subdirectory for each experiment group:

```
<project>/build/arm-rtems6-realview_pbx_a9_qemu/
└── <executable>.exe
```

---

## 4. Run Process

### 4.1 Running with QEMU

The project uses QEMU to emulate an ARM Cortex-A9 platform for running build artifacts.

**Basic run command:**

```bash
qemu-system-arm \
  -no-reboot \
  -nographic \
  -M realview-pbx-a9 \
  -m 256M \
  -audiodev none,id=none \
  -kernel <path-to.exe>
```

Parameter reference:

| Parameter | Description |
|-----------|-------------|
| `-no-reboot` | Do not reboot after program exit |
| `-nographic` | No graphical output; redirect serial to terminal |
| `-M realview-pbx-a9` | Emulate ARM RealView PBX-A9 board |
| `-m 256M` | Allocate 256 MB of RAM |
| `-audiodev none,id=none` | Disable audio device |
| `-kernel` | Path to the ELF executable to run |

**Exiting QEMU:**

Press `Ctrl+A` then `X`.

**Full run example (intra_process_demo):**

```bash
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && \
   qemu-system-arm \
     -no-reboot -nographic \
     -M realview-pbx-a9 -m 256M \
     -audiodev none,id=none \
     -kernel intra_process_demo/tmp/build/timer_manager.exe'
```

**Expected output sample:**

```
rtss_timer: driver initialized (major=4)
[TimeManager] Channel 0 started, period 500 ticks
[TimeManager] Channel 1 started, period 1000 ticks
[TimeManager] Started (RTEMS hardware tick interrupt mode), trigger mode: SEMAPHORE
[Task A] 500ms tick timer fired! count=0, ticks=1500, address=0x2A010
  [Consumer] Received timer event: value=0, address=0x2A010
[Task A] 500ms tick timer fired! count=1, ticks=2000, address=0x2A010
[Task B] 1000ms tick timer fired! count=0, ticks=2000, address=0x2A014
  [Consumer] Received timer event: value=1, address=0x2A010
  [Consumer] Received timer event: value=0, address=0x2A014
...
```

> **Zero-copy verification**: Producer and Consumer `address` values match, confirming intra-process communication is working.

### 4.2 Batch Experiment Execution

Two batch run scripts are provided.

**run_all.sh — Run all Exp_Inter experiments:**

```bash
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash run_all.sh'
```

- Runs 6 scheduling strategies × 6 chain lengths = 36 experiments
- Each experiment has a 300-second timeout
- Results automatically saved to `Results-<timestamp>/` directory
- CSV data is automatically extracted with average latency computed

**run_compare.sh — Run and compare core variants:**

```bash
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash run_compare.sh'
```

- Runs 5 strategies × 6 chain lengths = 30 experiments
- Automatically generates a comparison summary table
- Displays e2e (end-to-end) latency, pub latency, and sched latency

**Batch run sample output:**

```
=== Exp_Inter Full Comparison ===
Variants: NE RT MT Picas Ours
Chain lengths: 2 3 4 5 10 15
Output: /home/siweimutong/workspace/Results-compare-20260805-120000

--- NE (NoExecutor) ---
  [1/30] n=2 ... OK e2e=12500ns pub=3200ns sched=800ns
  [2/30] n=3 ... OK e2e=18400ns pub=3400ns sched=900ns
  ...

=== RESULTS ===
Var      Executor             n=2       n=3       n=4       n=5       n=10      n=15
NE       NoExecutor           12500     18400     24100     30200     58400     86700
RT       RTExecutor(rtss25)    5200      7800     10500     13100     25200     37300
MT       MultiThreaded         8100     12100     16200     20300     39100     58100
Picas    PiCAS                 4800      7200      9600     12000     23100     34200
Ours     RTExecutor(rtems)     3500      5200      7100      8800     17100     25400

Units: nanoseconds (e2e average latency)
```

### 4.3 Interpreting Results

**CSV data column reference (Exp_Inter experiments):**

| Column | Field | Description | Unit |
|--------|-------|-------------|------|
| 1 | `sample_id` | Sample index | — |
| 2 | `chain_length` | Chain length (number of callbacks) | — |
| 3 | `e2e_latency` | End-to-end latency | ns |
| 4 | `pub_latency` | Publish latency | ns |
| 5-8 | ... | Intermediate node latencies | ns |
| 9 | `sched_latency` | Scheduling latency | ns |

**Key metrics:**

- **e2e_latency (End-to-end latency)**: Total time from message publication to final subscriber receipt — the most important performance metric
- **pub_latency (Publish latency)**: Time overhead of the publish operation itself
- **sched_latency (Scheduling latency)**: Time from readiness to actual execution, reflecting scheduler responsiveness

---

## 5. Project Directory Structure

```
workspace/
├── README.md                       # This file (English version)
├── README-zh.md                    # Chinese version
├── RTEMS-README.md                 # RTEMS6 (Phytium D2000) environment deployment
├── RTEMS-DDS-README.md             # RTEMS 6.1 quick-start + DDS / ROS 2 component porting
├── build_all.sh                    # Unified build entry-point script
├── build_all_exp*.sh               # Per-experiment-family batch build scripts
├── run_all.sh                      # Exp_Inter batch runner script
├── run_all_exp*.sh                 # Per-experiment-family batch run scripts
├── run_compare.sh                  # Core variant comparison runner script
├── RTcolcon                        # Base library parallel build manager
│
├── ─── Base ROS 2 Libraries (27 packages) ───
├── rcutils/                        # C utility library (rcutils-5.1.5)
├── rcpputils/                      # C++ utility classes
├── ament_index_cpp/                # Package indexing and discovery
├── class_loader/                   # Dynamic library loading
├── rcl/                            # ROS Client Library C API (rcl-5.3.7)
├── rclcpp/                         # ROS Client Library C++ API (rclcpp-16.0.8)
├── rmw/                            # ROS Middleware Abstraction (rmw-6.1.1)
├── rmw_dds_common/                 # DDS common utilities
├── rmw_fastrtps/                   # Fast-DDS adaptation layer
├── rmw_implementation/             # RMW implementation selector
├── rosidl/                         # ROS IDL runtime and code generation
├── DDS/                            # Fast-CDR + Fast-DDS
├── spdlog/                         # High-performance C++ logging
├── yaml/                           # YAML parsing
├── logging/                        # Logging infrastructure
├── statistics_collector/           # Statistics collector
├── rcl_interfaces/                 # rcl message definitions
├── builtin_interfaces/             # Basic message types
├── std_msgs/                       # Standard messages
├── action_msgs/                    # Action message definitions
├── composition_interfaces/         # Node composition interfaces
├── lifecycle_msgs/                 # Lifecycle message definitions
├── rosgraph_msgs/                  # ROS graph messages
├── statistics_msgs/                # Statistics messages
├── example_interfaces/             # Example interface definitions
├── unique_identifier_msgs/         # UUID messages
├── ament_cmake/                    # Build system macros
│
├── ─── Custom Executors & Core Components ───
├── rclcpp-rtems/                   # Modified rclcpp (with RTExecutor + DDS notification)
│   └── rclcpp/executors/
│       ├── rt_executor.hpp/.cpp    # RTExecutor: kernel-driven callback dispatch
│       └── driver/
│           ├── rtss_timer_driver.h # Hardware timer driver API
│           └── rtss_timer_driver.c # Cortex-A9 PT driver implementation
├── rclcpp-picas/                   # Modified rclcpp (with PiCAS executor)
├── rclcpp-rtss25/                  # rclcpp-rtss25 baseline
├── RTExecutor/                     # RTExecutor standalone benchmark
├── RTROS/                          # RT-ROS (EventsCBGExecutor) benchmark
├── PICAS/                          # PiCAS standalone benchmark
│
├── ─── Application Examples ───
├── examples/                       # Basic ROS 2 examples
├── intra_process_demo/             # Intra-process zero-copy communication demo
├── pingpong_exp/                   # Ping-pong latency test
├── demos/                          # ROS 2 demo collection
├── lifecycle_exp/                  # Lifecycle node experiments
│
├── ─── Experiment Groups (scheduling strategy comparison) ───
├── Example/
│   ├── Exp_Intra-ST|MT|Ours|Picas|ROSRT/   # Exp_Intra series (intra-process scheduling)
│   └── Exp_Inter-ST|MT|RT|Ours|Picas|ROSRT/ # Exp_Inter series (inter-process scheduling)
│
├── ─── Auxiliary ───
├── paper/                          # Paper materials
└── Results-*/                      # Experiment result output directories
```

---

## 6. Command Quick Reference

### Container Management

```bash
# Start the container
docker start rtems6-dev

# Stop the container (stop when done to save resources)
docker stop rtems6-dev

# Check container status
docker ps -a | grep rtems6

# Enter an interactive shell inside the container (for debugging)
docker exec -it rtems6-dev bash

# Execute commands inside the container (recommended)
docker exec -i rtems6-dev bash -lc '<command>'
```

### Environment Verification

```bash
# Check if container is running
docker inspect -f '{{.State.Running}}' rtems6-dev

# Check toolchain
docker exec -i rtems6-dev bash -lc 'which arm-rtems6-gcc'

# Check QEMU
docker exec -i rtems6-dev bash -lc 'qemu-system-arm --version'

# Check BSP static libraries
docker exec -i rtems6-dev bash -lc \
  'ls $HOME/RTEMS/quick-start/rtems/6/arm-rtems6/realview_pbx_a9_qemu/lib/*.a | wc -l'
```

### Build

```bash
# Force full rebuild
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash build_all.sh force'

# Incremental build of base libraries
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash build_all.sh -l no-force'

# Build applications only
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash build_all.sh -a force'

# High-parallelism build
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash build_all.sh -j 8 -t 16 force'

# Build libraries but skip certain apps
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash build_all.sh -s examples -s exp force'
```

### Run

```bash
# Run a single executable
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && \
   qemu-system-arm -no-reboot -nographic \
     -M realview-pbx-a9 -m 256M \
     -audiodev none,id=none \
     -kernel <path-to.exe>'

# Run all experiments and collect results
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash run_all.sh'

# Run comparison experiments
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace && bash run_compare.sh'

# Run intra_process_demo
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace/intra_process_demo && \
   qemu-system-arm -no-reboot -nographic \
     -M realview-pbx-a9 -m 256M \
     -audiodev none,id=none \
     -kernel tmp/build/timer_manager.exe'
```

### Manual Single-Project Build

```bash
# Manual build (not via build_all.sh)
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace/intra_process_demo && \
   bash config/gen_run.sh && bash config/run.sh'

# Custom Waf build parameters
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace/intra_process_demo/tmp && \
   ./waf configure --rtems=$HOME/RTEMS/quick-start/rtems/6 \
     --rtems-bsp=arm/realview_pbx_a9_qemu && \
   ./waf build --targets=timer_manager.exe'
```

---

## 7. Troubleshooting

### 7.1 Docker Issues

**Container not running**

```bash
# Check container status
docker ps -a | grep rtems6-dev

# If status is Exited, start manually
docker start rtems6-dev
```

**Directory mount issues**

```bash
# Confirm host directory exists
ls ~/docker_ws/workspace/

# Confirm visibility inside container
docker exec -i rtems6-dev bash -lc 'ls /home/siweimutong/workspace/'

# If paths don't match, remove the old container and recreate
docker rm rtems6-dev
docker run -d --name rtems6-dev \
  -v ~/docker_ws:/home/siweimutong \
  -w /home/siweimutong/workspace \
  roker405/rtems6-env:v1.0 sleep infinity
```

### 7.2 Build Issues

**RTEMS toolchain not found**

```bash
# Confirm PATH includes RTEMS toolchain
docker exec -i rtems6-dev bash -lc 'echo $PATH'
# Should include /opt/rtems6/bin
```

**Base libraries not fully built**

```bash
# Confirm key static libraries exist
docker exec -i rtems6-dev bash -lc \
  'for lib in librclcpp.a librcl.a librmw.a libyaml.a librcutils.a; do
     echo -n "$lib: ";
     test -f $HOME/RTEMS/quick-start/rtems/6/arm-rtems6/realview_pbx_a9_qemu/lib/$lib && echo "OK" || echo "MISSING";
   done'
```

**Waf configure failure**

```bash
# Confirm RTEMS BSP path is correct
docker exec -i rtems6-dev bash -lc \
  'ls $HOME/RTEMS/quick-start/rtems/6/arm-rtems6/realview_pbx_a9_qemu/'

# Clean and retry
docker exec -i rtems6-dev bash -lc \
  'cd /home/siweimutong/workspace/<project>/tmp && \
   rm -rf ./.waf* ./build && \
   ./waf configure --rtems=$HOME/RTEMS/quick-start/rtems/6 \
     --rtems-bsp=arm/realview_pbx_a9_qemu'
```

**Duplicate confdefs.h inclusion**

Some projects using both `config/test_main.cpp` and `default-network-init.h` entry points may encounter duplicate `confdefs.h` inclusion. Resolution:
- Wrap `confdefs.h` at the end of source files with `#ifndef RTEMS_USE_DEFAULT_NETWORK_INIT`
- Add `-DRTEMS_USE_DEFAULT_NETWORK_INIT` to `cxxflags` in `wscript`

### 7.3 Runtime Issues

**No QEMU output**

```bash
# Confirm BSP name matching
# Build uses: realview_pbx_a9_qemu (underscores)
# QEMU runtime: realview-pbx-a9 (hyphens)

# Try explicitly specifying serial output
docker exec -i rtems6-dev bash -lc \
  'qemu-system-arm -M realview-pbx-a9 -m 256M -nographic \
     -serial mon:stdio -kernel <path-to.exe>'
```

**QEMU exits too quickly**

The program may be crashing during startup. Add debug output:

```bash
docker exec -i rtems6-dev bash -lc \
  'qemu-system-arm -M realview-pbx-a9 -m 256M -nographic \
     -d cpu_reset -D qemu_debug.log -kernel <path-to.exe>'
```

**Driver initialization failure**

```
rtss_timer: timer_create ch0 failed: too many
```

- Ensure `CONFIGURE_MAXIMUM_TIMERS >= RTSS_TIMER_MAX_CHANNELS` (8)
- When using `default-network-init.h` (with `CONFIGURE_UNLIMITED_OBJECTS`), this issue should not occur

**Intra-process communication not working (different addresses)**

- Ensure both nodes set `rclcpp::NodeOptions().use_intra_process_comms(true)`
- Ensure using `std::unique_ptr` + `std::move` for publishing
- Ensure Consumer callback parameter is `UniquePtr` (not const ref)

**Inaccurate timing**

- Check `CONFIGURE_MICROSECONDS_PER_TICK` setting (recommended: 1000 = 1ms)
- `default-network-init.h` defaults to 10000 (10ms) when this macro is unset
- Add `#define CONFIGURE_MICROSECONDS_PER_TICK 1000` before including `confdefs.h`

### 7.4 Debugging Tips

**Inspect build artifacts**

```bash
# View ELF header
docker exec -i rtems6-dev bash -lc \
  'arm-rtems6-objdump -f <path-to.exe>'

# View symbol table
docker exec -i rtems6-dev bash -lc \
  'arm-rtems6-nm <path-to.exe> | head -20'

# View section sizes
docker exec -i rtems6-dev bash -lc \
  'arm-rtems6-size <path-to.exe>'
```

**Step-through debugging with QEMU + GDB**

```bash
# Start QEMU and wait for GDB connection
docker exec -i rtems6-dev bash -lc \
  'qemu-system-arm -M realview-pbx-a9 -m 256M -nographic \
     -s -S -kernel <path-to.exe>'

# In another terminal, connect GDB
docker exec -it rtems6-dev bash -lc \
  'arm-rtems6-gdb <path-to.exe> -ex "target remote :1234"'
```

---

## References

- [RTEMS 6.1 Quick Start Guide](https://docs.rtems.org/branches/master/user/start/index.html)
- [RTEMS User Manual](https://docs.rtems.org/branches/master/user/index.html)
- [ROS 2 Documentation](https://docs.ros.org/en/humble/)
- [RTEMS Waf Build System](https://gitlab.com/rtems-community/rtems_waf)
- Project internal documentation:
  - `intra_process_demo/README.md` — Intra-process communication demo details
  - `rclcpp-rtems/README.md` — RTExecutor + DDS notification mechanism
  - `RTEMS-README.md` — RTEMS6 (Phytium D2000) environment deployment
  - `RTEMS-DDS-README.md` — RTEMS 6.1 quick-start + DDS / ROS 2 component porting
