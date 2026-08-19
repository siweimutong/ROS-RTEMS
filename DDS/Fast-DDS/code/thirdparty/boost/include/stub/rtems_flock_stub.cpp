// rtems_flock_stub.hpp —— RTEMS file lock interface stub functions
#include "stub/rtems_flock_stub.hpp"

// 2. Stub the flock() function (empty implementation, adapted to RTEMS lacking file locks)
extern "C" int flock(int fd, int operation) {
    (void)fd;         // Suppress unused parameter warning
    (void)operation;  // RTEMS has no cross-process file locks; empty implementation
    return 0;         // Return 0 for success to avoid Boost error handling
}
