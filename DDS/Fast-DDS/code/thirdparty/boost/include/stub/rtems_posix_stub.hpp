// rtems_posix_stub.hpp —— RTEMS 缺失 POSIX 接口桩函数
#ifndef RTEMS_POSIX_STUB_HPP
#define RTEMS_POSIX_STUB_HPP

// 2. 打桩：ftruncate 空实现（返回 0 表示成功，适配 Boost 错误处理）
extern "C" int ftruncate(int fd, off_t length);

// 3. 兜底：补充其他可能缺失的文件操作接口（按需）
extern "C" int truncate(const char* path, off_t length);

#endif // RTEMS_POSIX_STUB_HPP