// rtems_posix_stub.hpp -- stubs for POSIX interfaces missing on RTEMS

// 1. Include required headers (defines types such as off_t)
#include <sys/types.h>
#include <fcntl.h>

#include "stub/rtems_posix_stub.hpp"
#include <boost/interprocess/shared_memory_object.hpp>

// 2. Stub: no-op ftruncate implementation (returning 0 reports success, matching Boost error handling)
// extern "C" int ftruncate(int fd, off_t length) 
// {
//     (void)fd;     // suppress unused-parameter warning
//     (void)length; // RTEMS has no file truncation, so this is a no-op
//     return 0;     // return 0 so Boost believes truncation succeeded, avoiding a crash
// }

// 3. Fallback: add other file-operation interfaces that may be missing (as needed)
extern "C" int truncate(const char* path, off_t length) 
{
    (void)path;
    (void)length;
    return 0;
}

namespace boost {
namespace interprocess {

// 1. No-op implementation of shared_memory_object::truncate()
void shared_memory_object::truncate(long long size) {
    // No-op, used only to satisfy the linker
    // Shared memory may not be available in the real RTEMS environment
    (void)size;  // avoid unused-parameter warning
}
}
}
