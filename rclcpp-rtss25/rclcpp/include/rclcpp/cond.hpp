#pragma once

#include <unistd.h>
#include <stdint.h>
#include <atomic>

#ifdef RTEMS
#include <rtems.h>
#endif

namespace syncutil{

class Condition {

public:
    int wait_on(const uint32_t val);
    int set_val(const uint32_t val, const bool wakeup);

private:
    std::atomic<uint32_t> futex;
#ifdef RTEMS
    std::atomic<rtems_id> waiter_task{0};
#endif
    int futex_wait(const uint32_t val);
    int futex_wake();
};
}

