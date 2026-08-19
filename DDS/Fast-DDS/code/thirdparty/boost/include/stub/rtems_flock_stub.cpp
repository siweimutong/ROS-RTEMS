// rtems_flock_stub.hpp —— RTEMS 文件锁接口桩函数
#include "stub/rtems_flock_stub.hpp"

// 2. 打桩 flock() 函数（空实现，适配 RTEMS 无文件锁特性）
extern "C" int flock(int fd, int operation) {
    (void)fd;         // 屏蔽未使用参数警告
    (void)operation;  // RTEMS 无跨进程文件锁，空实现
    return 0;         // 返回 0 表示成功，避免 Boost 错误处理
}
