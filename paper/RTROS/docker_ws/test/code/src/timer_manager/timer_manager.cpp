/**
 * @file timer_manager.cpp
 *
 * RTEMS 6.1 ARM 内核定时器中断驱动 ROS2 应用
 *
 * 核心：rtems_timer_server_fire_after 由内核 tick ISR 的 Watchdog 触发，
 * Timer Server 任务设为最高优先级 (1)，可抢占所有低优先级任务。
 *
 * 每个 Timer 回调记录：
 *   - 理论释放时间 = 起始 tick + 周期 × 触发次数
 *   - 实际执行时间 = rtems_clock_get_ticks_since_boot()
 *   - 抖动 = 实际 - 理论
 *
 * 同时运行低优先级忙等任务，演示抢占效果。
 */

#include <rclcpp/rclcpp.hpp>
#include "std_msgs/msg/int32.hpp"
#include <chrono>
#include <functional>
#include <vector>
#include <memory>
#include <iostream>
#include <cstring>
#include <string>
#include <cinttypes>
#include <cstdio>

#include <rtems.h>

using namespace std::chrono_literals;

using Task = std::function<void()>;

static rtems_interval ms_to_ticks(uint32_t ms)
{
    rtems_interval tps = rtems_clock_get_ticks_per_second();
    return (rtems_interval)((uint64_t)ms * tps / 1000);
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

/* ---- 基于 RTEMS 内核定时器的定时器管理器 ---- */

#define TIMER_MAX_CHANNELS 8

/* 定时器触发记录 */
struct TriggerRecord {
    int            channel;
    uint32_t       fire_count;       /* 第几次触发 */
    rtems_interval theoretical_tick; /* 理论释放 tick */
    rtems_interval actual_tick;      /* 实际执行 tick */
    int32_t        jitter_ticks;     /* 抖动 (actual - theoretical) */
};

class TimeManager {
public:
    TimeManager()
        : running_(false), next_channel_(0), start_tick_(0),
          worker_task_id_(RTEMS_ID_NONE),
          busy_task_id_(RTEMS_ID_NONE)
    {}

    ~TimeManager() { stop(); }

    int register_timer(std::function<void()> callback, uint32_t period_ms)
    {
        if (next_channel_ >= TIMER_MAX_CHANNELS) {
            std::cerr << "[TimeManager] 通道数已达上限" << std::endl;
            return -1;
        }

        int ch = next_channel_++;
        auto info = std::make_unique<TimerInfo>();
        info->channel      = ch;
        info->callback     = std::move(callback);
        info->period_ticks = ms_to_ticks(period_ms);
        info->period_ms    = period_ms;
        info->timer_id     = RTEMS_ID_NONE;
        info->fire_count   = 0;

        timers_.push_back(std::move(info));
        return ch;
    }

    void start()
    {
        rtems_status_code sc;

        sc = ready_queue_.init('R', 'D');
        if (sc != RTEMS_SUCCESSFUL && sc != RTEMS_INCORRECT_STATE) {
            std::cerr << "[TimeManager] 队列初始化失败: "
                      << rtems_status_text(sc) << std::endl;
            return;
        }

        /* 启动 Timer Server，优先级设为 1（最高）以抢占所有低优先级任务 */
        sc = rtems_timer_initiate_server(
            1,  /* 最高优先级 */
            RTEMS_MINIMUM_STACK_SIZE * 2,
            RTEMS_DEFAULT_ATTRIBUTES);
        if (sc != RTEMS_SUCCESSFUL && sc != RTEMS_INCORRECT_STATE) {
            std::cerr << "[TimeManager] Timer Server 启动失败: "
                      << rtems_status_text(sc) << std::endl;
            return;
        }
        printf("[TimeManager] Timer Server 已启动，优先级=1 (最高)\n");

        start_tick_ = rtems_clock_get_ticks_since_boot();

        /* 为每个通道创建内核定时器并启动 */
        for (auto &info : timers_) {
            rtems_name tname = rtems_build_name('T', 'M', '0' + info->channel, ' ');
            sc = rtems_timer_create(tname, &info->timer_id);
            if (sc != RTEMS_SUCCESSFUL) {
                std::cerr << "[TimeManager] 创建定时器通道 "
                          << info->channel << " 失败: "
                          << rtems_status_text(sc) << std::endl;
                continue;
            }

            sc = rtems_timer_server_fire_after(
                info->timer_id,
                info->period_ticks,
                timer_isr_callback,
                info.get());
            if (sc != RTEMS_SUCCESSFUL) {
                std::cerr << "[TimeManager] 启动通道 "
                          << info->channel << " 失败: "
                          << rtems_status_text(sc) << std::endl;
                continue;
            }

            printf("[TimeManager] 通道 %d 已启动，周期=%ums (%u ticks)，"
                   "理论首次释放=%u\n",
                   info->channel, info->period_ms, info->period_ticks,
                   (unsigned)(start_tick_ + info->period_ticks));
        }

        instance_ = this;
        running_ = true;

        /* worker 任务：优先级 5，高优先级执行回调 */
        sc = rtems_task_create(
            rtems_build_name('W', 'O', 'R', 'K'),
            5,
            RTEMS_MINIMUM_STACK_SIZE * 4,
            RTEMS_DEFAULT_MODES,
            RTEMS_DEFAULT_ATTRIBUTES,
            &worker_task_id_);
        if (sc != RTEMS_SUCCESSFUL) {
            std::cerr << "[TimeManager] 创建 worker 任务失败" << std::endl;
            return;
        }
        rtems_task_start(worker_task_id_, worker_entry, (rtems_task_argument)this);

        /* 低优先级忙等任务：演示抢占 */
        sc = rtems_task_create(
            rtems_build_name('B', 'U', 'S', 'Y'),
            200,  /* 低优先级 */
            RTEMS_MINIMUM_STACK_SIZE * 2,
            RTEMS_DEFAULT_MODES,
            RTEMS_DEFAULT_ATTRIBUTES,
            &busy_task_id_);
        if (sc == RTEMS_SUCCESSFUL) {
            rtems_task_start(busy_task_id_, busy_task_entry, 0);
            printf("[TimeManager] 低优先级忙等任务已启动 (prio=200)，可被定时器抢占\n");
        }
    }

    void stop()
    {
        if (!running_) return;
        running_ = false;

        for (auto &info : timers_) {
            if (info->timer_id != RTEMS_ID_NONE) {
                rtems_timer_cancel(info->timer_id);
                rtems_timer_delete(info->timer_id);
                info->timer_id = RTEMS_ID_NONE;
            }
        }

        rtems_event_send(worker_task_id_, RTEMS_EVENT_1);
        rtems_event_send(busy_task_id_, RTEMS_EVENT_2);

        if (worker_task_id_ != RTEMS_ID_NONE) {
            rtems_task_delete(worker_task_id_);
            worker_task_id_ = RTEMS_ID_NONE;
        }
        if (busy_task_id_ != RTEMS_ID_NONE) {
            rtems_task_delete(busy_task_id_);
            busy_task_id_ = RTEMS_ID_NONE;
        }
    }

private:
    struct TimerInfo {
        int                    channel;
        std::function<void()>  callback;
        rtems_interval         period_ticks;
        uint32_t               period_ms;
        rtems_id               timer_id;
        volatile uint32_t      fire_count;
    };

    /* ---- 内核定时器回调：Timer Server 任务上下文 (prio=1) ---- */

    static void timer_isr_callback(rtems_id timer_id, void *arg)
    {
        TimerInfo *info = reinterpret_cast<TimerInfo *>(arg);
        if (!info || !info->callback) return;

        uint32_t count = info->fire_count + 1;
        info->fire_count = count;

        rtems_interval actual   = rtems_clock_get_ticks_since_boot();
        rtems_interval theory  = instance_->start_tick_ + count * info->period_ticks;
        int32_t        jitter  = (int32_t)(actual - theory);

        /* 封装触发记录 + 回调，推入就绪队列 */
        TriggerRecord rec;
        rec.channel         = info->channel;
        rec.fire_count      = count;
        rec.theoretical_tick = theory;
        rec.actual_tick      = actual;
        rec.jitter_ticks     = jitter;

        TimeManager *self = get_instance();
        if (self) {
            self->ready_queue_.push([info, rec]() {
                /* 打印抢占信息和时序分析 */
                rtems_task_priority prio;
                rtems_task_set_priority(RTEMS_SELF, RTEMS_CURRENT_PRIORITY, &prio);
                printf("[Ch%d] #%u | 理论释放=%u 实际执行=%u 抖动=%d tick | "
                       "执行优先级=%u (被高优先级抢占调度)\n",
                       rec.channel, rec.fire_count,
                       (unsigned)rec.theoretical_tick,
                       (unsigned)rec.actual_tick,
                       rec.jitter_ticks,
                       (unsigned)prio);
                info->callback();
            });
            rtems_event_send(self->worker_task_id_, RTEMS_EVENT_1);
        }

        /* 重新装填定时器 */
        rtems_timer_server_fire_after(
            timer_id, info->period_ticks, timer_isr_callback, arg);
    }

    /* ---- worker 任务：优先级 5 ---- */

    static rtems_task worker_entry(rtems_task_argument arg)
    {
        TimeManager *self = reinterpret_cast<TimeManager *>(arg);
        self->worker_loop();
        rtems_task_delete(RTEMS_SELF);
    }

    void worker_loop()
    {
        while (running_) {
            Task task;

            rtems_event_set events;
            rtems_status_code sc = rtems_event_receive(
                RTEMS_EVENT_1,
                RTEMS_EVENT_ANY | RTEMS_WAIT,
                RTEMS_NO_TIMEOUT,
                &events);
            if (sc != RTEMS_SUCCESSFUL || !running_)
                break;

            while (ready_queue_.try_pop(task)) {
                if (task) task();
            }
        }
    }

    /* ---- 低优先级忙等任务：演示抢占 ---- */

    static rtems_task busy_task_entry(rtems_task_argument arg)
    {
        (void)arg;
        volatile uint32_t counter = 0;
        rtems_event_set events;

        printf("[BUSY] 低优先级任务运行中 (prio=200)，等待被抢占...\n");

        while (true) {
            rtems_status_code sc = rtems_event_receive(
                RTEMS_EVENT_2,
                RTEMS_EVENT_ANY | RTEMS_NO_WAIT,
                0, &events);
            if (sc == RTEMS_SUCCESSFUL) break;

            counter++;
            /* 每约 5 秒打印一次 */
            if ((counter % 5000000) == 0) {
                printf("[BUSY] 计数=%u, ticks=%u (低优先级仍在跑)\n",
                       counter,
                       (unsigned)rtems_clock_get_ticks_since_boot());
            }
        }
        rtems_task_delete(RTEMS_SELF);
    }

    static TimeManager *get_instance() { return instance_; }
    static TimeManager *instance_;

    volatile bool      running_;
    int                next_channel_;
    rtems_interval     start_tick_;
    rtems_id           worker_task_id_;
    rtems_id           busy_task_id_;
    std::vector<std::unique_ptr<TimerInfo>> timers_;
    RtsemQueue         ready_queue_;
};

TimeManager *TimeManager::instance_ = nullptr;

/* ---- 定时器生产者节点 ---- */

struct TimerProducer : public rclcpp::Node
{
    TimerProducer(const std::string & name, const std::string & output)
    : Node(name, rclcpp::NodeOptions().use_intra_process_comms(true))
    {
        pub_ = this->create_publisher<std_msgs::msg::Int32>(output, 10);
        std::weak_ptr<std::remove_pointer<decltype(pub_.get())>::type> captured_pub = pub_;

        tm_ = std::make_unique<TimeManager>();

        /* 通道 0：500ms 周期，高优先级内核定时器 */
        tm_->register_timer(
            [this, captured_pub]() {
                auto pub_ptr = captured_pub.lock();
                if (!pub_ptr) { return; }
                static int32_t count_a = 0;
                auto msg = std::make_unique<std_msgs::msg::Int32>();
                msg->data = count_a++;
                pub_ptr->publish(std::move(msg));
            }, 500);

        /* 通道 1：1000ms 周期，高优先级内核定时器 */
        tm_->register_timer(
            [this, captured_pub]() {
                auto pub_ptr = captured_pub.lock();
                if (!pub_ptr) { return; }
                static int32_t count_b = 0;
                auto msg = std::make_unique<std_msgs::msg::Int32>();
                msg->data = count_b++;
                pub_ptr->publish(std::move(msg));
            }, 1000);

        /* 通道 2：200ms 周期，更高频率演示抢占 */
        tm_->register_timer(
            [this, captured_pub]() {
                auto pub_ptr = captured_pub.lock();
                if (!pub_ptr) { return; }
                static int32_t count_c = 0;
                auto msg = std::make_unique<std_msgs::msg::Int32>();
                msg->data = count_c++;
                pub_ptr->publish(std::move(msg));
            }, 200);

        tm_->start();
    }

    ~TimerProducer()
    {
        if (tm_) tm_->stop();
    }

    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr pub_;
    std::unique_ptr<TimeManager> tm_;
};

/* ---- 定时器消费者节点 ---- */

struct TimerConsumer : public rclcpp::Node
{
    TimerConsumer(const std::string & name, const std::string & input)
    : Node(name, rclcpp::NodeOptions().use_intra_process_comms(true))
    {
        sub_ = this->create_subscription<std_msgs::msg::Int32>(
            input,
            10,
            [](std_msgs::msg::Int32::UniquePtr msg) {
                printf("  [消费者] value=%d, addr=0x%" PRIXPTR "\n",
                       msg->data, reinterpret_cast<std::uintptr_t>(msg.get()));
            });
    }

    rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sub_;
};

/* ---- 入口函数 ---- */

int main(int argc, char * argv[])
{
    (void)argc;
    (void)argv;

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    rclcpp::init(argc, argv);

    printf("=== RTEMS 内核定时器抢占调度示例 ===\n");
    printf("调度策略：优先级抢占 (RTEMS DEFAULT_MODES)\n");
    printf("  Timer Server  prio=1   (最高，内核 tick ISR 驱动)\n");
    printf("  Worker 任务   prio=5   (高优先级执行回调)\n");
    printf("  ROS2 Executor prio=50  (中等)\n");
    printf("  Busy 任务     prio=200 (最低，被抢占)\n");
    printf("====================================\n");

    auto producer = std::make_shared<TimerProducer>("timer_producer", "timer_events");
    auto consumer = std::make_shared<TimerConsumer>("timer_consumer", "timer_events");

    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(producer);
    executor.add_node(consumer);
    executor.spin();

    rclcpp::shutdown();

    return 0;
}
