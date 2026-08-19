// rtems_shm_stub.hpp —— RTEMS shared memory interface stub functions
#ifndef RTEMS_SHM_STUB_HPP
#define RTEMS_SHM_STUB_HPP

// 2. Stub for RTEMS: empty shm_open implementation (returns invalid handle -1)
extern "C" int shm_open(const char* name, int oflag, mode_t mode);

// 3. Companion stub: shm_unlink (empty implementation, returns 0 for success)
extern "C" int shm_unlink(const char* name);

// 4. If Boost also uses mmap/munmap, add stubs as needed
#ifdef __cplusplus
extern "C" {
#endif
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);

int munmap(void* addr, size_t length);
#ifdef __cplusplus
}
#endif

#endif // RTEMS_SHM_STUB_HPP