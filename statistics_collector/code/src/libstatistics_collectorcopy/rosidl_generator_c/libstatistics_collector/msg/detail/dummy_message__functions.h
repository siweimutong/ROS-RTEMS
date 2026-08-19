// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from libstatistics_collector:msg/DummyMessage.idl
// generated code does not contain a copyright notice

#ifndef LIBSTATISTICS_COLLECTOR__MSG__DETAIL__DUMMY_MESSAGE__FUNCTIONS_H_
#define LIBSTATISTICS_COLLECTOR__MSG__DETAIL__DUMMY_MESSAGE__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "libstatistics_collector/msg/rosidl_generator_c__visibility_control.h"

#include "libstatistics_collector/msg/detail/dummy_message__struct.h"

/// Initialize msg/DummyMessage message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * libstatistics_collector__msg__DummyMessage
 * )) before or use
 * libstatistics_collector__msg__DummyMessage__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
bool
libstatistics_collector__msg__DummyMessage__init(libstatistics_collector__msg__DummyMessage * msg);

/// Finalize msg/DummyMessage message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
void
libstatistics_collector__msg__DummyMessage__fini(libstatistics_collector__msg__DummyMessage * msg);

/// Create msg/DummyMessage message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * libstatistics_collector__msg__DummyMessage__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
libstatistics_collector__msg__DummyMessage *
libstatistics_collector__msg__DummyMessage__create();

/// Destroy msg/DummyMessage message.
/**
 * It calls
 * libstatistics_collector__msg__DummyMessage__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
void
libstatistics_collector__msg__DummyMessage__destroy(libstatistics_collector__msg__DummyMessage * msg);

/// Check for msg/DummyMessage message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
bool
libstatistics_collector__msg__DummyMessage__are_equal(const libstatistics_collector__msg__DummyMessage * lhs, const libstatistics_collector__msg__DummyMessage * rhs);

/// Copy a msg/DummyMessage message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
bool
libstatistics_collector__msg__DummyMessage__copy(
  const libstatistics_collector__msg__DummyMessage * input,
  libstatistics_collector__msg__DummyMessage * output);

/// Initialize array of msg/DummyMessage messages.
/**
 * It allocates the memory for the number of elements and calls
 * libstatistics_collector__msg__DummyMessage__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
bool
libstatistics_collector__msg__DummyMessage__Sequence__init(libstatistics_collector__msg__DummyMessage__Sequence * array, size_t size);

/// Finalize array of msg/DummyMessage messages.
/**
 * It calls
 * libstatistics_collector__msg__DummyMessage__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
void
libstatistics_collector__msg__DummyMessage__Sequence__fini(libstatistics_collector__msg__DummyMessage__Sequence * array);

/// Create array of msg/DummyMessage messages.
/**
 * It allocates the memory for the array and calls
 * libstatistics_collector__msg__DummyMessage__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
libstatistics_collector__msg__DummyMessage__Sequence *
libstatistics_collector__msg__DummyMessage__Sequence__create(size_t size);

/// Destroy array of msg/DummyMessage messages.
/**
 * It calls
 * libstatistics_collector__msg__DummyMessage__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
void
libstatistics_collector__msg__DummyMessage__Sequence__destroy(libstatistics_collector__msg__DummyMessage__Sequence * array);

/// Check for msg/DummyMessage message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
bool
libstatistics_collector__msg__DummyMessage__Sequence__are_equal(const libstatistics_collector__msg__DummyMessage__Sequence * lhs, const libstatistics_collector__msg__DummyMessage__Sequence * rhs);

/// Copy an array of msg/DummyMessage messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_libstatistics_collector
bool
libstatistics_collector__msg__DummyMessage__Sequence__copy(
  const libstatistics_collector__msg__DummyMessage__Sequence * input,
  libstatistics_collector__msg__DummyMessage__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // LIBSTATISTICS_COLLECTOR__MSG__DETAIL__DUMMY_MESSAGE__FUNCTIONS_H_
