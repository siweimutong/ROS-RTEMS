// rtems_shm_stub.hpp —— RTEMS 共享内存接口桩函数

// 1. 引入必要头文件（定义 mode_t/oflag 等类型）
#include <fcntl.h>   // O_RDWR/O_CREAT 等宏
#include <sys/stat.h>// mode_t 类型
#include <unistd.h>  // close 函数

#include "stub/rtems_shm_stub.hpp"

// 2. 为 RTEMS 打桩：空实现 shm_open（返回无效句柄 -1）
extern "C" int shm_open(const char* name, int oflag, mode_t mode) 
{
    // RTEMS 无跨进程共享内存，直接返回无效句柄
    (void)name;  // 屏蔽未使用参数警告
    (void)oflag;
    (void)mode;
    return -1;   // 无效文件句柄，适配 Boost 错误处理逻辑
}

// 3. 配套桩函数：shm_unlink（空实现，返回 0 表示成功）
extern "C" int shm_unlink(const char* name) 
{
    (void)name;
    return 0;
}

// 4. 若 Boost 还用到 mmap/munmap，补充桩函数（按需）
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
    return nullptr; // 返回映射失败
}
int munmap(void* addr, size_t length) {
    (void)addr;
    (void)length;
    return 0;
}
#ifdef __cplusplus
}
#endif
