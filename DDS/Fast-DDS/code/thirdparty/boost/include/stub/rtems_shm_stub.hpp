// rtems_shm_stub.hpp —— RTEMS 共享内存接口桩函数
#ifndef RTEMS_SHM_STUB_HPP
#define RTEMS_SHM_STUB_HPP

// 2. 为 RTEMS 打桩：空实现 shm_open（返回无效句柄 -1）
extern "C" int shm_open(const char* name, int oflag, mode_t mode);

// 3. 配套桩函数：shm_unlink（空实现，返回 0 表示成功）
extern "C" int shm_unlink(const char* name);

// 4. 若 Boost 还用到 mmap/munmap，补充桩函数（按需）
#ifdef __cplusplus
extern "C" {
#endif
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);

int munmap(void* addr, size_t length);
#ifdef __cplusplus
}
#endif

#endif // RTEMS_SHM_STUB_HPP