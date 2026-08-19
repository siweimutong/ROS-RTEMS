// rtems_flock_stub.hpp -- stub for the RTEMS file-lock interface
#ifndef RTEMS_FLOCK_STUB_HPP
#define RTEMS_FLOCK_STUB_HPP

// 1. Define the missing flock() flag macros (POSIX standard values)
#ifndef LOCK_EX
#define LOCK_EX 1    // exclusive lock (write lock)
#endif
#ifndef LOCK_SH
#define LOCK_SH 2    // shared lock (read lock)
#endif
#ifndef LOCK_UN
#define LOCK_UN 8    // release lock
#endif
#ifndef LOCK_NB
#define LOCK_NB 4    // non-blocking lock (the macro that caused the error)
#endif
extern "C" int flock(int fd, int operation);

#endif // RTEMS_FLOCK_STUB_HPP