#ifndef TRACETOOLS__TRACETOOLS_H_
#define TRACETOOLS__TRACETOOLS_H_

/* Stub tracetools header for RTEMS build — all tracing macros are no-ops */

#define TRACEPOINT(...) ((void)0)

#define TRACETOOLS_TRACEPOINT_ENABLED(name) false

inline bool tracetools_tracepoint_enabled(const char * name) {
  (void)name;
  return false;
}

#endif  /* TRACETOOLS__TRACETOOLS_H_ */
