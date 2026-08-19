# intra_process_demo - RTEMS 6.1 ARM 硬件 Tick 中断驱动的 ROS2 定时器

| | |
|---|---|
| **开发人员** | siweimutong |
| **联系方式** | luoxiantong@bjut.edu.cn |

通过 RTEMS 6.1 的 `rtems_timer_fire_after` 在 Clock Tick ISR 上下文中触发定时回调，经由 RTEMS 设备驱动框架投递到应用层，驱动 ROS2 定时回调逻辑。

## 架构

```
┌─────────────────────────────────────────────────────┐
│                RTEMS 6.1 内核                        │
│                                                     │
│  ARM Generic Timer 硬件中断                          │
│         │                                           │
│         ▼                                           │
│  Clock Tick ISR                                     │
│         │                                           │
│         ▼                                           │
│  rtems_timer_fire_after callback (ISR 上下文)       │
│  ┌──────────┐  ┌──────────┐                         │
│  │ ch0 500ms│  │ ch1 1000ms│                        │
│  └────┬─────┘  └────┬─────┘                         │
│       └──────┬──────┘                               │
│              ▼                                       │
│       事件环形缓冲区 (256 slots)                      │
│              │                                       │
│       rtems_event_send / sem_release / mq_send       │
│              │                                       │
│     /dev/rtss_timer (RTEMS 设备驱动)                 │
└──────────────┬──────────────────────────────────────┘
               │  read() 阻塞
               ▼
┌─────────────────────────────────────────────────────┐
│          应用层 (RTEMS 任务 + ROS2 节点)             │
│                                                     │
│  Reader 任务 (pri=80)                                │
│     │ read(/dev/rtss_timer) → 就绪队列               │
│     ▼                                                │
│  Worker 任务 (pri=70)                                │
│     │ rtems_event_receive 触发                        │
│     ▼                                                │
│  TimerProducer::callback → publish(UniquePtr) ──┐    │
│                                                 │    │
│     ─ ─ ─ ─ intra-process 零拷贝 ─ ─ ─ ─ ─ ─ ─┘    │
│                                                 │    │
│  TimerConsumer::subscription_callback ◄──────────┘    │
│     │ printf(事件数据)                                │
│                                                     │
│  MultiThreadedExecutor (Init 任务)                   │
└─────────────────────────────────────────────────────┘
```

## 目录结构

```
intra_process_demo/
├── config/
│   ├── gen_run.sh                  # 生成 waf 构建环境
│   ├── run.sh                      # 一键编译
│   └── test_main.cpp               # RTEMS 入口：Init() → test_main() → main()
├── code/
│   ├── CMakeLists.txt              # ament_cmake 构建文件
│   ├── package.xml                 # ROS2 包描述
│   ├── src/
│   │   ├── timer_manager/
│   │   │   └── timer_manager.cpp   # ★ Tick 中断 ROS2 应用
│   │   ├── two_node_pipeline/
│   │   │   └── two_node_pipeline.cpp
│   │   ├── cyclic_pipeline/
│   │   │   └── cyclic_pipeline.cpp
│   │   └── image_pipeline/
│   │       ├── camera_node.cpp
│   │       ├── watermark_node.cpp
│   │       ├── image_view_node.cpp
│   │       └── ...
│   ├── include/
│   └── test/
├── include/
│   └── default-network-init.h      # RTEMS Init 任务 + confdefs.h
└── README.md
```

外部依赖：

```
RTSS_RTROS/
└── driver/
    ├── rtss_timer_driver.h         # 定时器驱动接口（RTEMS 类型）
    └── rtss_timer_driver.c         # RTEMS 设备驱动实现
```

---

## 一、前置条件

### 1.1 编译主机

| 项目 | 要求 |
|------|------|
| 操作系统 | Ubuntu 20.04 或 22.04 LTS |
| 磁盘空间 | >= 10 GB |
| 依赖包 | `build-essential git python3 python3-pip texinfo bison flex` |

```bash
sudo apt update
sudo apt install -y build-essential git python3 python3-pip texinfo bison flex
```

### 1.2 RTEMS 6.1 工具链与 BSP

假设已按 [RTEMS 快速入门](https://docs.rtems.org/branches/master/user/start/index.html) 安装：

```bash
# 工具链
arm-rtems6-gcc --version

# BSP（本项目中 config/run.sh 使用 realview_pbx_a9_qemu）
ls $HOME/RTEMS/quick-start/rtems/6/
```

### 1.3 ROS2 交叉编译库

`intra_process_demo` 已将 ROS2 核心库交叉编译为 RTEMS ARM 静态库，位于 `wscript` 中
`lib=[]` 列出的各个 `.a` 文件。确保这些库已构建并安装到 RTEMS BSP 搜索路径中。

---

## 二、构建

### 2.1 生成 waf 构建环境

```bash
cd /path/to/RTSS_RTROS/intra_process_demo

# 生成 tmp/ 目录，包含 waf、rtems_waf 子模块、init.c、wscript
bash config/gen_run.sh
```

此脚本会：
1. 创建 `tmp/` 目录
2. 下载 `waf-2.0.19`
3. 初始化 git 仓库，添加 `rtems_waf` 子模块
4. 生成 `init.c`（RTEMS 最小配置）
5. 生成 `wscript`（构建描述，含源文件、头文件路径、链接库列表）

### 2.2 修改 wscript 添加 timer_manager

`gen_run.sh` 生成的 `wscript` 默认只编译 `two_node_pipeline`。需手动添加 `timer_manager`
的源文件和驱动：

```python
# 在 wscript 的 bld.program() 中修改：

bld.program(
    target="timer_manager.exe",
    source=[
        "../test_main.cpp",                                        # RTEMS 入口
        "../../code/src/timer_manager/timer_manager.cpp",          # 应用
        "../../../../driver/rtss_timer_driver.c",                  # 驱动
    ],
    includes=[
        "../../code/include",
        "../../../../driver",                                      # 驱动头文件
        # ... (其他 ROS2 头文件路径与 two_node_pipeline 相同)
    ],
    cxxflags="-DRTEMS -Wno-psabi -fexceptions -fPIC",
    linkflags="-static",
    lib=[
        # ... (与 two_node_pipeline 相同的 ROS2 库列表)
    ],
)
```

### 2.3 一键编译

```bash
cd /path/to/RTSS_RTROS/intra_process_demo

# 执行编译（config/run.sh 内部调用 waf configure + waf build）
bash config/run.sh
```

`run.sh` 内容：

```bash
cd tmp
rm -rf ./.waf* ./build
./waf configure --rtems=$HOME/RTEMS/quick-start/rtems/6 --rtems-bsp=arm/realview_pbx_a9_qemu
./waf build
```

编译成功后，产物位于 `tmp/build/` 目录下。

### 2.4 手动编译（调试用）

```bash
cd /path/to/RTSS_RTROS/intra_process_demo/tmp

# 配置
./waf configure --rtems=$HOME/RTEMS/quick-start/rtems/6 \
    --rtems-bsp=arm/realview_pbx_a9_qemu

# 仅编译 timer_manager 目标
./waf build --targets=timer_manager.exe

# 查看产物
arm-rtems6-objdump -f build/timer_manager.exe
```

---

## 三、部署与运行

### 3.1 QEMU 仿真

```bash
# 安装 QEMU ARM
sudo apt install qemu-system-arm

# 运行 timer_manager
qemu-system-arm \
    -M realview-pbx-a9 \
    -m 256M \
    -nographic \
    -kernel tmp/build/timer_manager.exe
```

> `Ctrl+A X` 退出 QEMU。

### 3.2 启动流程

QEMU 或硬件上电后：

1. RTEMS 内核初始化
2. `Init()` 任务运行（`default-network-init.h` 提供）→ 初始化网络 → 调用 `test_main()`
3. `test_main()` 构造 argc/argv → 调用 `main()`
4. `main()` 中 `rclcpp::init(argc, argv)` → 创建 ROS2 节点 → executor.spin()
5. `TimerProducer` 构造时创建 `TimeManager`，打开 `/dev/rtss_timer`，启动 tick 定时器
6. Reader 任务从驱动 `read()` 阻塞等待 tick 中断事件
7. Worker 任务执行回调 → `TimerProducer::publish()` → `TimerConsumer` 收到事件

### 3.3 预期输出

```
rtss_timer: driver initialized (major=4)
[TimeManager] 通道 0 已启动，周期 500 ticks
[TimeManager] 通道 1 已启动，周期 1000 ticks
[TimeManager] 启动（RTEMS 硬件 tick 中断模式），触发模式: SEMAPHORE
[任务A] 500ms tick定时器触发！count=0, ticks=1500, address=0x2A010
  [消费者] 收到定时器事件: value=0, address=0x2A010
[任务A] 500ms tick定时器触发！count=1, ticks=2000, address=0x2A010
[任务B] 1000ms tick定时器触发！count=0, ticks=2000, address=0x2A014
  [消费者] 收到定时器事件: value=1, address=0x2A010
  [消费者] 收到定时器事件: value=0, address=0x2A014
[任务A] 500ms tick定时器触发！count=2, ticks=2500, address=0x2A010
  [消费者] 收到定时器事件: value=2, address=0x2A010
...
```

**零拷贝验证**：Producer 和 Consumer 的 address 相同，说明 intra-process 通信生效，消息在同一进程内传递未经序列化。

### 3.4 退出

QEMU 中 `Ctrl+A` 然后 `X`。

实机上 RTEMS 应用通常不主动退出。如需优雅关闭，需触发 `TimerProducer` 析构 → `TimeManager::stop()` → 停止所有通道 → 关闭驱动 → `rclcpp::shutdown()`。

---

## 四、代码详解

### 4.1 入口点链

```
Init()                          ← default-network-init.h，RTEMS 初始任务
  │
  ├─ rtems_bsd_initialize()     ← 初始化 BSD 网络栈
  ├─ default_network_ifconfig() ← 配置网络接口
  └─ test_main()                ← config/test_main.cpp
       │
       └─ main(argc, argv)      ← timer_manager.cpp 的 main()
            │
            ├─ rclcpp::init()
            ├─ 创建 TimerProducer / TimerConsumer
            └─ executor.spin()
```

### 4.2 定时器驱动（driver/rtss_timer_driver.c）

| 函数 | 说明 |
|------|------|
| `rtss_driver_initialize` | BSP 启动时调用：创建 8 个 `rtems_timer`，注册 `/dev/rtss_timer` |
| `rtss_driver_open` | 记录调用者任务 ID（EVENT 模式用） |
| `rtss_driver_read` | 阻塞读取 tick 中断事件，根据模式选择等待机制 |
| `rtss_driver_control` | ioctl：SET_MODE / SET_PERIOD / START / STOP / GET_COUNT |
| `timer_isr_callback` | ISR 上下文：入环形缓冲区 + 通知读取任务 |

### 4.3 应用层组件（timer_manager.cpp）

| 组件 | 说明 |
|------|------|
| `TimeManager` | 定时器管理器：注册通道 → `open(/dev/rtss_timer)` → `ioctl` 配置 → 创建 reader/worker 任务 |
| `RtsemQueue` | 基于 `rtems_semaphore` 的线程安全任务队列 |
| `TimerProducer` | ROS2 生产者节点：tick 中断回调 → `UniquePtr` publish → 零拷贝 |
| `TimerConsumer` | ROS2 消费者节点：`UniquePtr` subscribe → 打印事件数据 |

### 4.4 完整数据流

```
ARM Generic Timer IRQ
    │
    ▼
Clock Tick ISR ─── rtems_timer_fire_after ──► timer_isr_callback()
                                                    │
                                        ┌───────────┼───────────┐
                                        ▼           ▼           ▼
                                   EVENT模式    SEM模式      MQ模式
                                        │           │           │
                                  ring_buf push  ring_buf push  mq_send
                                        │           │           │
                                  event_send   sem_release   (直接投递)
                                        │           │           │
                                        └─────┬─────┘           │
                                              ▼                 │
                                    /dev/rtss_timer            │
                                    read_entry (阻塞等待)       │
                                              │                 │
                                              ▼                 ▼
                                        Reader 任务 ◄───────────┘
                                              │
                                         RtsemQueue.push()
                                              │
                                         rtems_event_send(EVENT_1)
                                              │
                                              ▼
                                        Worker 任务
                                              │
                                         callback() ──► TimerProducer::publish()
                                                              │
                                                   intra-process (零拷贝)
                                                              │
                                                              ▼
                                                   TimerConsumer::subscription_callback()
```

---

## 五、配置说明

### 5.1 RTEMS 系统配置

配置位于 `timer_manager.cpp` 底部 `confdefs.h` 包含之前：

```c
#define CONFIGURE_MICROSECONDS_PER_TICK    1000    /* 1ms/tick */

#define CONFIGURE_MAXIMUM_TIMERS           8       /* = RTSS_TIMER_MAX_CHANNELS */
#define CONFIGURE_MAXIMUM_TASKS            8       /* init + reader + worker + ROS2 内部 */
#define CONFIGURE_MAXIMUM_SEMAPHORES       8       /* 队列 + 触发 + ROS2 */
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES   4       /* MESSAGE_QUEUE 模式 */
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 8       /* open /dev/rtss_timer */

#define CONFIGURE_APPLICATION_EXTRA_DRIVERS rtss_timer_driver_table
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_NULL_DRIVER
```

> **注意**：当使用 `config/test_main.cpp` + `default-network-init.h` 入口时，
> `confdefs.h` 已由 `default-network-init.h` 末尾包含，其中使用了
> `CONFIGURE_UNLIMITED_OBJECTS`，此时 `timer_manager.cpp` 中的 `confdefs.h`
> 不应重复包含。需在编译时通过条件编译或移除 `timer_manager.cpp` 底部的
> `confdefs.h` 段来避免冲突。

### 5.2 定时器通道

```cpp
TimeManager tm("semaphore");

tm.register_timer(callback1, 500);   // 通道 0，500ms = 500 ticks
tm.register_timer(callback2, 1000);  // 通道 1，1000ms = 1000 ticks
```

最多 8 个通道（`RTSS_TIMER_MAX_CHANNELS`），自动递增分配。

### 5.3 触发模式

| 模式 | 关键词 | RTEMS IPC | 特点 |
|------|--------|-----------|------|
| EVENT | `"event"` | `rtems_event_send/receive` | 轻量，ISR 安全，32 位事件集 |
| SEMAPHORE | `"semaphore"` | `rtems_semaphore_release/obtain` | 每次中断精确唤醒一次 worker（**默认**） |
| MESSAGE_QUEUE | `"message_queue"` | `rtems_message_queue_send/receive` | 带数据传递，适合回调耗时不均场景 |

### 5.4 任务优先级

| 任务 | 优先级 | 说明 |
|------|--------|------|
| Reader | 80 | 高优先级，及时读取中断事件 |
| Worker | 70 | 执行 ROS2 回调（publish） |
| Init | 50 | MultiThreadedExecutor 主循环 |

RTEMS 优先级：数值越小越高。可根据负载调整 `rtems_task_create` 的 `initial_priority`。

---

## 六、timer_manager 与 time_manager 对比

| 对比项 | 旧版 `time_manager.cpp`（根目录） | 新版 `timer_manager.cpp` |
|--------|----------------------------------|--------------------------|
| 位置 | `RTSS_RTROS/time_manager.cpp` | `intra_process_demo/code/src/timer_manager/` |
| ROS2 初始化 | `rclcpp::init(0, nullptr)` | `rclcpp::init(argc, argv)` 构造有效参数 |
| 节点模式 | 单个 `MyNode`（仅 printf） | `TimerProducer` + `TimerConsumer`（pub/sub） |
| 事件循环 | `while(rclcpp::ok()) { rtems_task_wake_after }` | `MultiThreadedExecutor::spin()` |
| 进程内通信 | 无 | `use_intra_process_comms(true)` 零拷贝 |
| 消息传递 | 无 | `UniquePtr` + `std::move` → `UniquePtr` 接收 |
| 定时器驱动 | `/dev/rtss_timer` (RTEMS) | `/dev/rtss_timer` (RTEMS) **完全一致** |
| TimeManager | reader/worker 任务 | reader/worker 任务 **完全一致** |
| confdefs.h | 内嵌在源文件末尾 | 内嵌（使用 test_main.cpp 入口时需移除） |

---

## 七、扩展指南

### 7.1 添加新的定时器通道

在 `TimerProducer` 构造函数中增加 `register_timer`：

```cpp
tm_->register_timer(
    [this, captured_pub]() {
        auto pub_ptr = captured_pub.lock();
        if (!pub_ptr) { return; }
        static int32_t count_c = 0;
        auto msg = std::make_unique<std_msgs::msg::Int32>();
        msg->data = count_c++;
        printf("[任务C] 200ms tick定时器触发！count=%d\n", msg->data);
        pub_ptr->publish(std::move(msg));
    }, 200);
```

### 7.2 切换触发模式

```cpp
auto tm = std::make_unique<TimeManager>("event");         // 事件位
auto tm = std::make_unique<TimeManager>("semaphore");     // 信号量（默认）
auto tm = std::make_unique<TimeManager>("message_queue"); // 消息队列
```

### 7.3 同时编译多个 demo

在 `wscript` 中定义多个 `bld.program()` 目标：

```python
bld.program(target="two_node_pipeline.exe", source=[...], ...)
bld.program(target="timer_manager.exe", source=[...], ...)
```

选择性编译：

```bash
./waf build --targets=timer_manager.exe
```

---

## 八、故障排查

### rclcpp::init 初始化失败

```
terminate called after throwing an instance of 'std::runtime_error'
  what():  failed to initialize rcl
```

原因：原始代码使用 `rclcpp::init(0, nullptr)`，部分 ROS2 实现要求 argc >= 1。
修复：使用构造的 argc/argv：

```cpp
char arg0[] = "timer_manager";
char * argv[] = { arg0, nullptr };
int argc = 1;
rclcpp::init(argc, argv);
```

### confdefs.h 重复包含

使用 `config/test_main.cpp` + `default-network-init.h` 入口时，`confdefs.h` 已被包含。
此时 `timer_manager.cpp` 底部的 `confdefs.h` 段会导致重复定义错误。

修复方案：将 `timer_manager.cpp` 底部的 RTEMS 配置段（`extern "C" { ... #include <rtems/confdefs.h> }`）
注释掉或用条件编译包裹：

```cpp
#ifndef RTEMS_USE_DEFAULT_NETWORK_INIT
extern "C" {
// ... confdefs.h 配置 ...
#include <rtems/confdefs.h>
}
#endif
```

在 wscript 的 `cxxflags` 中添加 `-DRTEMS_USE_DEFAULT_NETWORK_INIT`。

### 驱动初始化失败

```
rtss_timer: timer_create ch0 failed: too many
```

`CONFIGURE_MAXIMUM_TIMERS` 需 >= `RTSS_TIMER_MAX_CHANNELS` (8)。
若使用 `default-network-init.h` 的 `CONFIGURE_UNLIMITED_OBJECTS`，此问题不会出现。

### 无法打开 /dev/rtss_timer

- 确认 `CONFIGURE_APPLICATION_EXTRA_DRIVERS rtss_timer_driver_table` 已定义
- 确认 `CONFIGURE_APPLICATION_NEEDS_NULL_DRIVER` 已启用（IMFS 依赖）
- 若使用 `default-network-init.h`，确认 `CONFIGURE_IMFS_ENABLE_MKFIFO` 已定义

### 定时不精确

- 检查 `CONFIGURE_MICROSECONDS_PER_TICK`（建议 1000 = 1ms）
- 若使用 `default-network-init.h`，其未设置此宏，默认为 10000 (10ms)
- 需在包含 `confdefs.h` 之前定义：`#define CONFIGURE_MICROSECONDS_PER_TICK 1000`

### intra-process 通信未生效（地址不同）

- 确认两个节点都设置了 `rclcpp::NodeOptions().use_intra_process_comms(true)`
- 确认使用 `UniquePtr` + `std::move` 发布
- 确认 Consumer 回调参数为 `UniquePtr`（非 const ref）

### waf configure 找不到 RTEMS

```bash
# 确认 RTEMS 安装路径
ls $HOME/RTEMS/quick-start/rtems/6/

# 修改 config/run.sh 中的 --rtems 路径
./waf configure --rtems=/your/actual/rtems/path --rtems-bsp=arm/realview_pbx_a9_qemu
```

### QEMU 无输出

- 确认 `-M` 参数与 BSP 匹配：`realview-pbx-a9` 对应 `realview_pbx_a9_qemu`
- 确认使用 `-nographic` 参数
- 尝试 `-serial mon:stdio` 显式指定串口
