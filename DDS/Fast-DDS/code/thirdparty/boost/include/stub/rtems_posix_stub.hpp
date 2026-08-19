// rtems_posix_stub.hpp —— Stub functions for POSIX interfaces missing on RTEMS
#ifndef RTEMS_POSIX_STUB_HPP
#define RTEMS_POSIX_STUB_HPP

// 2. Stub: empty ftruncate implementation (returns 0 for success, compatible with Boost error handling)
extern "C" int ftruncate(int fd, off_t length);

// 3. Fallback: add other file operation interfaces that may be missing (as needed)
extern "C" int truncate(const char* path, off_t length);

#endif // RTEMS_POSIX_STUB_HPP