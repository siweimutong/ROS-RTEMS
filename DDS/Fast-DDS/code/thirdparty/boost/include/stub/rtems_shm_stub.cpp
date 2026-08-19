// rtems_shm_stub.hpp —— RTEMS shared memory interface stub functions

// 1. Include required headers (defines types such as mode_t/oflag)
#include <fcntl.h>   // Macros such as O_RDWR/O_CREAT
#include <sys/stat.h>// mode_t type
#include <unistd.h>  // close function

#include "stub/rtems_shm_stub.hpp"

// 2. Stub for RTEMS: empty shm_open implementation (returns invalid handle -1)
extern "C" int shm_open(const char* name, int oflag, mode_t mode) 
{
    // RTEMS has no cross-process shared memory; return an invalid handle directly
    (void)name;  // Suppress unused parameter warning
    (void)oflag;
    (void)mode;
    return -1;   // Invalid file handle, compatible with Boost error handling logic
}

// 3. Companion stub: shm_unlink (empty implementation, returns 0 for success)
extern "C" int shm_unlink(const char* name) 
{
    (void)name;
    return 0;
}

// 4. If Boost also uses mmap/munmap, add stubs as needed
#ifdef __cplusplus
extern "C" {
#endif
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    (void)addr;
    (void)length;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;
    return nullptr; // Indicate mapping failure
}
int munmap(void* addr, size_t length) {
    (void)addr;
    (void)length;
    return 0;
}
#ifdef __cplusplus
}
#endif
