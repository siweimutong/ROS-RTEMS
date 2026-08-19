// rtems_posix_stub.hpp —— RTEMS 缺失 POSIX 接口桩函数

// 1. 引入必要头文件（定义 off_t 等类型）
#include <sys/types.h>
#include <fcntl.h>

#include "stub/rtems_posix_stub.hpp"
#include <boost/interprocess/shared_memory_object.hpp>

// 2. 打桩：ftruncate 空实现（返回 0 表示成功，适配 Boost 错误处理）
// extern "C" int ftruncate(int fd, off_t length) 
// {
//     (void)fd;     // 屏蔽未使用参数警告
//     (void)length; // RTEMS 无文件截断功能，空实现
//     return 0;     // 返回 0 让 Boost 认为截断成功，避免崩溃
// }

// 3. 兜底：补充其他可能缺失的文件操作接口（按需）
extern "C" int truncate(const char* path, off_t length) 
{
    (void)path;
    (void)length;
    return 0;
}

namespace boost {
namespace interprocess {

// 1. shared_memory_object::truncate() 空实现
void shared_memory_object::truncate(long long size) {
    // 空实现，仅用于通过链接
    // 在实际 RTEMS 环境中，共享内存可能不可用
    (void)size;  // 避免未使用参数警告
}
}
}
