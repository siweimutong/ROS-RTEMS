#ifdef RTEMS
/* RTEMS: use rtems_event for Condition instead of Linux futex */
#include <rtems.h>
#include <unistd.h>
#include <atomic>

#include "rclcpp/cond.hpp"

int syncutil::Condition::futex_wait(const uint32_t val) {
    (void)val;
    rtems_event_set events;
    return rtems_event_receive(
        RTEMS_EVENT_1,
        RTEMS_EVENT_ANY | RTEMS_WAIT,
        RTEMS_NO_TIMEOUT,
        &events) == RTEMS_SUCCESSFUL ? 0 : -1;
}

int syncutil::Condition::futex_wake() {
    rtems_id task = waiter_task.load(std::memory_order_acquire);
    if (task != 0) {
        rtems_event_send(task, RTEMS_EVENT_1);
    }
    return 0;
}

int syncutil::Condition::wait_on(const uint32_t val) {
    waiter_task.store(rtems_task_self(), std::memory_order_release);
    while (futex.load(std::memory_order_relaxed) == val) {
        rtems_event_set events;
        rtems_status_code sc = rtems_event_receive(
            RTEMS_EVENT_1,
            RTEMS_EVENT_ANY | RTEMS_WAIT,
            1,  /* 1 tick timeout to re-check condition */
            &events);
        if (sc != RTEMS_SUCCESSFUL && sc != RTEMS_TIMEOUT) {
            waiter_task.store(0, std::memory_order_release);
            return -1;
        }
    }
    return 0;
}

int syncutil::Condition::set_val(const uint32_t val, const bool wakeup) {
    futex.store(val, std::memory_order_release);
    if (wakeup) {
        futex_wake();
    }
    return 0;
}

#else
/* Linux: original futex implementation */
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/futex.h>
#include <atomic>

#include "rclcpp/cond.hpp"

inline int syncutil::Condition::futex_wait(const uint32_t val) {
    return syscall(SYS_futex, &(this->futex), FUTEX_WAIT, val, NULL);
}

inline int syncutil::Condition::futex_wake() {
    return syscall(SYS_futex, &(this->futex), FUTEX_WAKE, 1, NULL);
}

int syncutil::Condition::wait_on(const uint32_t val) {
    int err;
    while (futex.load(std::memory_order_relaxed) == val) {
        if (err = futex_wait(val)) {
            return err;
        }
    }
    return 0;
}

int syncutil::Condition::set_val(const uint32_t val, const bool wakeup) {
    futex.store(val, std::memory_order_release);
    if (wakeup) {
        return futex_wake();
    }
    return 0;
}
#endif
