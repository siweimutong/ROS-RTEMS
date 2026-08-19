/**
 * @file timer_manager.cpp
 *
 * RTEMS 6.1 ARM real-time timer-interrupt ROS2 application
 *
 * Reads hardware tick interrupt events via the RTEMS device driver
 * /dev/rtss_timer, driving the ROS2 timer callback logic.
 *
 * Analogous to the two_node_pipeline pattern:
 *   - TimerProducer : timer-interrupt-driven producer node
 *   - TimerConsumer : event consumer node
 *   - Uses intra-process communication with zero-copy messaging
 *
 * The trigger mode is implemented in the driver layer and selected
 * via the mode keyword:
 *   - "event"             : event bit
 *   - "semaphore"         : binary semaphore
 *   - "message_queue"     : message queue
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

/* Include the driver's shared header */
extern "C" {
#include "driver/rtss_timer_driver.h"
}

using namespace std::chrono_literals;

using Task = std::function<void()>;

/* ---- RTEMS helpers ---- */

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

    std::cerr << "[TimeManager] unknown mode '" << mode
              << "', falling back to EVENT" << std::endl;
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

/* ---- RTEMS semaphore-based task-safe queue ---- */

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

/* ---- RTEMS tick-driven timer manager ---- */

class TimeManager {
public:
    /**
     * @param mode trigger mode keyword: "event" / "semaphore" / "message_queue"
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
            std::cerr << "[TimeManager] channel limit reached" << std::endl;
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

        /* Initialize the internal ready queue */
        sc = ready_queue_.init('R', 'D');
        if (sc != RTEMS_SUCCESSFUL) {
            std::cerr << "[TimeManager] queue initialization failed: "
                      << status_text(sc) << std::endl;
            return;
        }

        /* Open the device driver */
        dev_fd_ = open(RTSS_DEVICE_NAME, O_RDWR);
        if (dev_fd_ < 0) {
            std::cerr << "[TimeManager] cannot open " << RTSS_DEVICE_NAME
                      << " (" << strerror(errno) << ")" << std::endl;
            return;
        }

        /* Set the driver-layer trigger mode via ioctl */
        rtss_timer_mode_config mode_cfg;
        mode_cfg.mode = trigger_mode_;
        if (ioctl(dev_fd_, RTSS_TIMER_SET_MODE, &mode_cfg) < 0) {
            std::cerr << "[TimeManager] SET_MODE failed, falling back to EVENT" << std::endl;
            trigger_mode_ = RTSS_MODE_EVENT;
        }

        /* Configure and start the timer channels */
        for (auto &info : timers_) {
            rtss_timer_config cfg;
            cfg.channel      = info->channel;
            cfg.period_ticks = info->period_ticks;

            if (ioctl(dev_fd_, RTSS_TIMER_SET_PERIOD, &cfg) < 0) {
                std::cerr << "[TimeManager] SET_PERIOD failed for channel "
                          << cfg.channel << std::endl;
                continue;
            }

            if (ioctl(dev_fd_, RTSS_TIMER_START, &cfg) < 0) {
                std::cerr << "[TimeManager] START failed for channel "
                          << cfg.channel << std::endl;
                continue;
            }

            std::cout << "[TimeManager] channel " << cfg.channel
                      << " started, period " << cfg.period_ticks
                      << " ticks" << std::endl;
        }

        running_ = true;

        /* Create the reader task: reads tick interrupt events from the driver */
        sc = rtems_task_create(
            rtems_build_name('R', 'E', 'A', 'D'),
            80,
            RTEMS_MINIMUM_STACK_SIZE * 4,
            RTEMS_DEFAULT_MODES,
            RTEMS_DEFAULT_ATTRIBUTES,
            &reader_task_id_);
        if (sc != RTEMS_SUCCESSFUL) {
            std::cerr << "[TimeManager] failed to create reader task: "
                      << status_text(sc) << std::endl;
            return;
        }

        sc = rtems_task_start(
            reader_task_id_,
            reader_task_entry,
            (rtems_task_argument)this);

        /* Create the worker task: executes callbacks */
        sc = rtems_task_create(
            rtems_build_name('W', 'O', 'R', 'K'),
            70,
            RTEMS_MINIMUM_STACK_SIZE * 4,
            RTEMS_DEFAULT_MODES,
            RTEMS_DEFAULT_ATTRIBUTES,
            &worker_task_id_);
        if (sc != RTEMS_SUCCESSFUL) {
            std::cerr << "[TimeManager] failed to create worker task: "
                      << status_text(sc) << std::endl;
            return;
        }

        sc = rtems_task_start(
            worker_task_id_,
            worker_task_entry,
            (rtems_task_argument)this);

        std::cout << "[TimeManager] started (RTEMS hardware tick interrupt mode), trigger mode: "
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

        /* Wake the worker so it exits */
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

    /* ---- reader task: reads tick interrupt events from the driver ---- */

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

            /* Find the callback for this channel and push it onto the ready queue */
            for (auto &info : timers_) {
                if (info->channel == evt.channel && info->callback) {
                    ready_queue_.push(info->callback);
                    rtems_event_send(worker_task_id_, RTEMS_EVENT_1);
                    break;
                }
            }
        }
    }

    /* ---- worker task: executes callbacks ---- */

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

            /* Wait for notification from the reader */
            rtems_event_set events;
            rtems_status_code sc = rtems_event_receive(
                RTEMS_EVENT_1,
                RTEMS_EVENT_ANY | RTEMS_WAIT,
                RTEMS_NO_TIMEOUT,
                &events);
            if (sc != RTEMS_SUCCESSFUL || !running_)
                break;

            /* Execute all callbacks in the ready queue */
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

/* ---- Timer producer node: driven by tick interrupts, publishes event messages ---- */

struct TimerProducer : public rclcpp::Node
{
    TimerProducer(const std::string & name, const std::string & output)
    : Node(name, rclcpp::NodeOptions().use_intra_process_comms(true))
    {
        pub_ = this->create_publisher<std_msgs::msg::Int32>(output, 10);
        std::weak_ptr<std::remove_pointer<decltype(pub_.get())>::type> captured_pub = pub_;

        tm_ = std::make_unique<TimeManager>("semaphore");

        /* Channel 0: 500 ms tick timer */
        tm_->register_timer(
            [this, captured_pub]() {
                auto pub_ptr = captured_pub.lock();
                if (!pub_ptr) { return; }
                static int32_t count_a = 0;
                auto msg = std::make_unique<std_msgs::msg::Int32>();
                msg->data = count_a++;
                printf("[Task A] 500ms tick timer fired! count=%d, ticks=%u, address=0x%"
                       PRIXPTR "\n",
                       msg->data,
                       (unsigned)rtems_clock_get_ticks_since_boot(),
                       reinterpret_cast<std::uintptr_t>(msg.get()));
                pub_ptr->publish(std::move(msg));
            }, 500);

        /* Channel 1: 1000 ms tick timer */
        tm_->register_timer(
            [this, captured_pub]() {
                auto pub_ptr = captured_pub.lock();
                if (!pub_ptr) { return; }
                static int32_t count_b = 0;
                auto msg = std::make_unique<std_msgs::msg::Int32>();
                msg->data = count_b++;
                printf("[Task B] 1000ms tick timer fired! count=%d, ticks=%u, address=0x%"
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

/* ---- Timer consumer node: receives timer event messages ---- */

struct TimerConsumer : public rclcpp::Node
{
    TimerConsumer(const std::string & name, const std::string & input)
    : Node(name, rclcpp::NodeOptions().use_intra_process_comms(true))
    {
        sub_ = this->create_subscription<std_msgs::msg::Int32>(
            input,
            10,
            [](std_msgs::msg::Int32::UniquePtr msg) {
                printf("  [Consumer] received timer event: value=%d, address=0x%" PRIXPTR "\n",
                       msg->data, reinterpret_cast<std::uintptr_t>(msg.get()));
            });
    }

    rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sub_;
};

/* ---- RTEMS init task + ROS2 initialization ---- */

extern "C" {

void Init(rtems_task_argument arg);

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_NULL_DRIVER

/* Register our timer driver */
#define CONFIGURE_APPLICATION_EXTRA_DRIVERS rtss_timer_driver_table

/* System configuration */
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 8
#define CONFIGURE_MAXIMUM_TIMERS           RTSS_TIMER_MAX_CHANNELS
#define CONFIGURE_MAXIMUM_SEMAPHORES       8
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES   4
#define CONFIGURE_MAXIMUM_TASKS            8
#define CONFIGURE_MICROSECONDS_PER_TICK    1000  /* 1 ms/tick */

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

    /* Build argc/argv to work around the original rclcpp::init(0, nullptr) issue */
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
