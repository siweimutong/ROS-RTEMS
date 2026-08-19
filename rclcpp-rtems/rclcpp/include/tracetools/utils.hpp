#ifndef TRACETOOLS__UTILS_HPP_
#define TRACETOOLS__UTILS_HPP_

/* Stub tracetools/utils.hpp for RTEMS build */

#include <cstdint>

namespace tracetools
{

inline const char * get_symbol(void * func_ptr)
{
  (void)func_ptr;
  return "unknown";
}

}  // namespace tracetools

#endif  /* TRACETOOLS__UTILS_HPP_ */
