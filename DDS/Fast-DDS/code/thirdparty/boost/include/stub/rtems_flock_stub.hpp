// rtems_flock_stub.hpp —— RTEMS 文件锁接口桩函数
#ifndef RTEMS_FLOCK_STUB_HPP
#define RTEMS_FLOCK_STUB_HPP

// 1. 补全 flock() 标志位宏定义（POSIX 标准值）
#ifndef LOCK_EX
#define LOCK_EX 1    // 排他锁（写锁）
#endif
#ifndef LOCK_SH
#define LOCK_SH 2    // 共享锁（读锁）
#endif
#ifndef LOCK_UN
#define LOCK_UN 8    // 释放锁
#endif
#ifndef LOCK_NB
#define LOCK_NB 4    // 非阻塞锁（核心报错的宏）
#endif
extern "C" int flock(int fd, int operation);

#endif // RTEMS_FLOCK_STUB_HPP