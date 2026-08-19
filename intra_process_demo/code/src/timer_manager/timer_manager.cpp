/**
 * @file timer_manager.cpp
 *
 * RTEMS 6.1 ARM 实时定时中断 ROS2 应用
 *
 * 通过 RTEMS 设备驱动 /dev/rtss_timer 读取硬件 tick 中断事件，
 * 驱动 ROS2 定时回调逻辑。
 *
 * 类比 two_node_pipeline 模式：
 *   - TimerProducer : 定时器中断驱动的生产者节点
 *   - TimerConsumer : 事件消费者节点
 *   - 使用 intra-process communication 零拷贝通信
 *
 * 触发模式由驱动层实现，通过 mode 关键词选择：
 *   - "event"             : 事件位
 *   - "semaphore"         : 二值信号量
 *   - "message_queue"     : 消息队列
 */

#include <rclcpp/rclcpp.hpp>
#include <chrono>
#include "std_msgs/msg/int32.hpp"
#include <functional>
#include <vector>
#include <memory>
#include <iostream>
#include <cstring>
#include <string>
#include <cinttypes>
#include <cstdio>

#include <rtems.h>
#include <rtems/libio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* 包含驱动共享头文件 */
extern "C" {
#include "driver/rtss_timer_driver.h"
}

using namespace std::chrono_literals;

using Task = std::function<void()>;

/* ---- RTEMS 辅助 ---- */

static const char *status_text(rtems_status_code sc)
{
    return rtems_status_text(sc);
}

static rtems_interval ms_to_ticks(uint32_t ms)
{
    rtems_interval tps = rtems_clock_get_ticks_per_second();
    return (rtems_interval)((uint64_t)ms * tps / 1000);
}

static rtss_trigger_mode parse_mode(const std::string &mode)
{
    if (mode == "event" || mode == "EVENT")
        return RTSS_MODE_EVENT;
    if (mode == "semaphore" || mode == "SEMAPHORE")
        return RTSS_MODE_BINARY_SEMAPHORE;
    if (mode == "message_queue" || mode == "MESSAGE_QUEUE")
        return RTSS_MODE_MESSAGE_QUEUE;

    std::cerr << "[TimeManager] 未知模式 '" << mode
              << "'，回退到 EVENT" << std::endl;
    return RTSS_MODE_EVENT;
}

static const char *mode_to_string(rtss_trigger_mode mode)
{
    switch (mode) {
    case RTSS_MODE_EVENT:            return "EVENT";
    case RTSS_MODE_BINARY_SEMAPHORE: return "BINARY_SEMAPHORE";
    case RTSS_MODE_MESSAGE_QUEUE:    return "MESSAGE_QUEUE";
    default:                         return "UNKNOWN";
    }
}

/* ---- 基于 RTEMS 信号量的任务安全队列 ---- */

class RtsemQueue {
public:
    RtsemQueue() : sem_id_(RTEMS_ID_NONE) {}

    ~RtsemQueue() {
        if (sem_id_ != RTEMS_ID_NONE)
            rtems_semaphore_delete(sem_id_);
    }

    rtems_status_code init(const char n1, const char n2)
    {
        rtems_name name = rtems_build_name(n1, n2, 'R', 'Q');
        return rtems_semaphore_create(
            name, 1,
            RTEMS_PRIORITY | RTEMS_BINARY_SEMAPHORE | RTEMS_INHERIT_PRIORITY,
            0, &sem_id_);
    }

    void push(Task task) {
        rtems_semaphore_obtain(sem_id_, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
        queue_.push_back(std::move(task));
        rtems_semaphore_release(sem_id_);
    }

    bool try_pop(Task &task) {
        rtems_status_code sc = rtems_semaphore_obtain(
            sem_id_, RTEMS_NO_WAIT, 0);
        if (sc != RTEMS_SUCCESSFUL)
            return false;

        if (queue_.empty()) {
            rtems_semaphore_release(sem_id_);
            return false;
        }

        task = std::move(queue_.front());
        queue_.erase(queue_.begin());
        rtems_semaphore_release(sem_id_);
        return true;
    }

private:
    rtems_id           sem_id_;
    std::vector<Task>  queue_;
};

/* ---- 基于 RTEMS tick 驱动的定时器管理器 ---- */

class TimeManager {
public:
    /**
     * @param mode 触发模式关键词："event" / "semaphore" / "message_queue"
     */
    TimeManager(const std::string &mode)
        : trigger_mode_(parse_mode(mode)),
          running_(false), dev_fd_(-1), next_channel_(0),
          reader_task_id_(RTEMS_ID_NONE),
          worker_task_id_(RTEMS_ID_NONE)
    {}

    ~TimeManager() { stop(); }

    int register_timer(std::function<void()> callback, uint32_t period_ms)
    {
        if (next_channel_ >= RTSS_TIMER_MAX_CHANNELS) {
            std::cerr << "[TimeManager] 通道数已达上限" << std::endl;
            return -1;
        }

        int ch = next_channel_++;
        auto info = std::make_unique<TimerInfo>();
        info->channel      = ch;
        info->callback     = std::move(callback);
        info->period_ticks = ms_to_ticks(period_ms);

        timers_.push_back(std::move(info));
        return ch;
    }

    void start()
    {
        rtems_status_code sc;

        /* 初始化内部就绪队列 */
        sc = ready_queue_.init('R', 'D');
        if (sc != RTEMS_SUCCESSFUL) {
            std::cerr << "[TimeManager] 队列初始化失败: "
                      << status_text(sc) << std::endl;
            return;
        }

        /* 打开设备驱动 */
        dev_fd_ = open(RTSS_DEVICE_NAME, O_RDWR);
        if (dev_fd_ < 0) {
            std::cerr << "[TimeManager] 无法打开 " << RTSS_DEVICE_NAME
                      << " (" << strerror(errno) << ")" << std::endl;
            return;
        }

        /* 通过 ioctl 设置驱动层触发模式 */
        rtss_timer_mode_config mode_cfg;
        mode_cfg.mode = trigger_mode_;
        if (ioctl(dev_fd_, RTSS_TIMER_SET_MODE, &mode_cfg) < 0) {
            std::cerr << "[TimeManager] SET_MODE 失败，回退到 EVENT" << std::endl;
            trigger_mode_ = RTSS_MODE_EVENT;
        }

        /* 配置并启动定时器通道 */
        for (auto &info : timers_) {
            rtss_timer_config cfg;
            cfg.channel      = info->channel;
            cfg.period_ticks = info->period_ticks;

            if (ioctl(dev_fd_, RTSS_TIMER_SET_PERIOD, &cfg) < 0) {
                std::cerr << "[TimeManager] SET_PERIOD 通道 "
                          << cfg.channel << " 失败" << std::endl;
                continue;
            }

            if (ioctl(dev_fd_, RTSS_TIMER_START, &cfg) < 0) {
                std::cerr << "[TimeManager] START 通道 "
                          << cfg.channel << " 失败" << std::endl;
                continue;
            }

            std::cout << "[TimeManager] 通道 " << cfg.channel
                      << " 已启动，周期 " << cfg.period_ticks
                      << " ticks" << std::endl;
        }

        running_ = true;

        /* 创建 reader 任务：从驱动读取 tick 中断事件 */
        sc = rtems_task_create(
            rtems_build_name('R', 'E', 'A', 'D'),
            80,
            RTEMS_MINIMUM_STACK_SIZE * 4,
            RTEMS_DEFAULT_MODES,
            RTEMS_DEFAULT_ATTRIBUTES,
            &reader_task_id_);
        if (sc != RTEMS_SUCCESSFUL) {
            std::cerr << "[TimeManager] 创建 reader 任务失败: "
                      << status_text(sc) << std::endl;
            return;
        }

        sc = rtems_task_start(
            reader_task_id_,
            reader_task_entry,
            (rtems_task_argument)this);

        /* 创建 worker 任务：执行回调 */
        sc = rtems_task_create(
            rtems_build_name('W', 'O', 'R', 'K'),
            70,
            RTEMS_MINIMUM_STACK_SIZE * 4,
            RTEMS_DEFAULT_MODES,
            RTEMS_DEFAULT_ATTRIBUTES,
            &worker_task_id_);
        if (sc != RTEMS_SUCCESSFUL) {
            std::cerr << "[TimeManager] 创建 worker 任务失败: "
                      << status_text(sc) << std::endl;
            return;
        }

        sc = rtems_task_start(
            worker_task_id_,
            worker_task_entry,
            (rtems_task_argument)this);

        std::cout << "[TimeManager] 启动（RTEMS 硬件 tick 中断模式），触发模式: "
                  << mode_to_string(trigger_mode_) << std::endl;
    }

    void stop()
    {
        if (!running_) return;
        running_ = false;

        for (auto &info : timers_) {
            rtss_timer_config cfg;
            cfg.channel      = info->channel;
            cfg.period_ticks = 0;
            if (dev_fd_ >= 0)
                ioctl(dev_fd_, RTSS_TIMER_STOP, &cfg);
        }

        if (dev_fd_ >= 0) {
            close(dev_fd_);
            dev_fd_ = -1;
        }

        /* 唤醒 worker 使其退出 */
        rtems_event_send(worker_task_id_, RTEMS_EVENT_1);

        if (reader_task_id_ != RTEMS_ID_NONE) {
            rtems_task_delete(reader_task_id_);
            reader_task_id_ = RTEMS_ID_NONE;
        }
        if (worker_task_id_ != RTEMS_ID_NONE) {
            rtems_task_delete(worker_task_id_);
            worker_task_id_ = RTEMS_ID_NONE;
        }
    }

private:
    struct TimerInfo {
        int                    channel;
        std::function<void()>  callback;
        rtems_interval         period_ticks;
    };

    /* ---- reader 任务：从驱动读取 tick 中断事件 ---- */

    static rtems_task reader_task_entry(rtems_task_argument arg)
    {
        TimeManager *self = reinterpret_cast<TimeManager *>(arg);
        self->reader_loop();
        rtems_task_delete(RTEMS_SELF);
    }

    void reader_loop()
    {
        rtss_timer_event evt;

        while (running_) {
            ssize_t n = read(dev_fd_, &evt, sizeof(evt));
            if (n < 0) {
                if (!running_) break;
                continue;
            }
            if ((size_t)n != sizeof(evt))
                continue;

            /* 根据通道号找到回调，推入就绪队列 */
            for (auto &info : timers_) {
                if (info->channel == evt.channel && info->callback) {
                    ready_queue_.push(info->callback);
                    rtems_event_send(worker_task_id_, RTEMS_EVENT_1);
                    break;
                }
            }
        }
    }

    /* ---- worker 任务：执行回调 ---- */

    static rtems_task worker_task_entry(rtems_task_argument arg)
    {
        TimeManager *self = reinterpret_cast<TimeManager *>(arg);
        self->worker_loop();
        rtems_task_delete(RTEMS_SELF);
    }

    void worker_loop()
    {
        while (running_) {
            Task task;

            /* 等待 reader 发来的通知 */
            rtems_event_set events;
            rtems_status_code sc = rtems_event_receive(
                RTEMS_EVENT_1,
                RTEMS_EVENT_ANY | RTEMS_WAIT,
                RTEMS_NO_TIMEOUT,
                &events);
            if (sc != RTEMS_SUCCESSFUL || !running_)
                break;

            /* 执行就绪队列中的所有回调 */
            while (ready_queue_.try_pop(task)) {
                if (task) task();
            }
        }
    }

    rtss_trigger_mode  trigger_mode_;
    volatile bool      running_;
    int                dev_fd_;
    int                next_channel_;

    rtems_id           reader_task_id_;
    rtems_id           worker_task_id_;

    std::vector<std::unique_ptr<TimerInfo>> timers_;
    RtsemQueue         ready_queue_;
};

/* ---- 定时器生产者节点：通过 tick 中断驱动，发布事件消息 ---- */

struct TimerProducer : public rclcpp::Node
{
    TimerProducer(const std::string & name, const std::string & output)
    : Node(name, rclcpp::NodeOptions().use_intra_process_comms(true))
    {
        pub_ = this->create_publisher<std_msgs::msg::Int32>(output, 10);
        std::weak_ptr<std::remove_pointer<decltype(pub_.get())>::type> captured_pub = pub_;

        tm_ = std::make_unique<TimeManager>("semaphore");

        /* 通道 0：500ms tick 定时器 */
        tm_->register_timer(
            [this, captured_pub]() {
                auto pub_ptr = captured_pub.lock();
                if (!pub_ptr) { return; }
                static int32_t count_a = 0;
                auto msg = std::make_unique<std_msgs::msg::Int32>();
                msg->data = count_a++;
                printf("[任务A] 500ms tick定时器触发！count=%d, ticks=%u, address=0x%"
                       PRIXPTR "\n",
                       msg->data,
                       (unsigned)rtems_clock_get_ticks_since_boot(),
                       reinterpret_cast<std::uintptr_t>(msg.get()));
                pub_ptr->publish(std::move(msg));
            }, 500);

        /* 通道 1：1000ms tick 定时器 */
        tm_->register_timer(
            [this, captured_pub]() {
                auto pub_ptr = captured_pub.lock();
                if (!pub_ptr) { return; }
                static int32_t count_b = 0;
                auto msg = std::make_unique<std_msgs::msg::Int32>();
                msg->data = count_b++;
                printf("[任务B] 1000ms tick定时器触发！count=%d, ticks=%u, address=0x%"
                       PRIXPTR "\n",
                       msg->data,
                       (unsigned)rtems_clock_get_ticks_since_boot(),
                       reinterpret_cast<std::uintptr_t>(msg.get()));
                pub_ptr->publish(std::move(msg));
            }, 1000);

        tm_->start();
    }

    ~TimerProducer()
    {
        if (tm_) tm_->stop();
    }

    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr pub_;
    std::unique_ptr<TimeManager> tm_;
};

/* ---- 定时器消费者节点：接收定时器事件消息 ---- */

struct TimerConsumer : public rclcpp::Node
{
    TimerConsumer(const std::string & name, const std::string & input)
    : Node(name, rclcpp::NodeOptions().use_intra_process_comms(true))
    {
        sub_ = this->create_subscription<std_msgs::msg::Int32>(
            input,
            10,
            [](std_msgs::msg::Int32::UniquePtr msg) {
                printf("  [消费者] 收到定时器事件: value=%d, address=0x%" PRIXPTR "\n",
                       msg->data, reinterpret_cast<std::uintptr_t>(msg.get()));
            });
    }

    rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sub_;
};

/* ---- RTEMS 初始任务 + ROS2 初始化 ---- */

extern "C" {

void Init(rtems_task_argument arg);

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_NULL_DRIVER

/* 注册我们的定时器驱动 */
#define CONFIGURE_APPLICATION_EXTRA_DRIVERS rtss_timer_driver_table

/* 系统配置 */
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 8
#define CONFIGURE_MAXIMUM_TIMERS           RTSS_TIMER_MAX_CHANNELS
#define CONFIGURE_MAXIMUM_SEMAPHORES       8
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES   4
#define CONFIGURE_MAXIMUM_TASKS            8
#define CONFIGURE_MICROSECONDS_PER_TICK    1000  /* 1ms/tick */

#define CONFIGURE_INIT_TASK_NAME           rtems_build_name('M', 'A', 'I', 'N')
#define CONFIGURE_INIT_TASK_PRIORITY       50
#define CONFIGURE_INIT_TASK_INITIAL_MODES  RTEMS_DEFAULT_MODES
#define CONFIGURE_INIT_TASK_ARGUMENTS      0
#define CONFIGURE_INIT_TASK_STACK_SIZE     (64 * 1024)
#define CONFIGURE_INIT_TASK_ENTRY_POINT    Init

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT
#include <rtems/confdefs.h>

}

void Init(rtems_task_argument arg)
{
    (void)arg;

    /* 构造 argc/argv，修复原始 rclcpp::init(0, nullptr) 初始化问题 */
    char arg0[] = "timer_manager";
    char * argv[] = { arg0, nullptr };
    int argc = 1;

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    rclcpp::init(argc, argv);

    auto producer = std::make_shared<TimerProducer>("timer_producer", "timer_events");
    auto consumer = std::make_shared<TimerConsumer>("timer_consumer", "timer_events");

    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(producer);
    executor.add_node(consumer);
    executor.spin();

    rclcpp::shutdown();
    rtems_task_delete(RTEMS_SELF);
}
