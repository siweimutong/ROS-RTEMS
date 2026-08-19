// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from libstatistics_collector:msg/DummyCustomHeaderMessage.idl
// generated code does not contain a copyright notice
#include "libstatistics_collector/msg/detail/dummy_custom_header_message__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `header`
#include "std_msgs/msg/detail/color_rgba__functions.h"

bool
libstatistics_collector__msg__DummyCustomHeaderMessage__init(libstatistics_collector__msg__DummyCustomHeaderMessage * msg)
{
  if (!msg) {
    return false;
  }
  // header
  if (!std_msgs__msg__ColorRGBA__init(&msg->header)) {
    libstatistics_collector__msg__DummyCustomHeaderMessage__fini(msg);
    return false;
  }
  return true;
}

void
libstatistics_collector__msg__DummyCustomHeaderMessage__fini(libstatistics_collector__msg__DummyCustomHeaderMessage * msg)
{
  if (!msg) {
    return;
  }
  // header
  std_msgs__msg__ColorRGBA__fini(&msg->header);
}

bool
libstatistics_collector__msg__DummyCustomHeaderMessage__are_equal(const libstatistics_collector__msg__DummyCustomHeaderMessage * lhs, const libstatistics_collector__msg__DummyCustomHeaderMessage * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // header
  if (!std_msgs__msg__ColorRGBA__are_equal(
      &(lhs->header), &(rhs->header)))
  {
    return false;
  }
  return true;
}

bool
libstatistics_collector__msg__DummyCustomHeaderMessage__copy(
  const libstatistics_collector__msg__DummyCustomHeaderMessage * input,
  libstatistics_collector__msg__DummyCustomHeaderMessage * output)
{
  if (!input || !output) {
    return false;
  }
  // header
  if (!std_msgs__msg__ColorRGBA__copy(
      &(input->header), &(output->header)))
  {
    return false;
  }
  return true;
}

libstatistics_collector__msg__DummyCustomHeaderMessage *
libstatistics_collector__msg__DummyCustomHeaderMessage__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  libstatistics_collector__msg__DummyCustomHeaderMessage * msg = (libstatistics_collector__msg__DummyCustomHeaderMessage *)allocator.allocate(sizeof(libstatistics_collector__msg__DummyCustomHeaderMessage), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(libstatistics_collector__msg__DummyCustomHeaderMessage));
  bool success = libstatistics_collector__msg__DummyCustomHeaderMessage__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
libstatistics_collector__msg__DummyCustomHeaderMessage__destroy(libstatistics_collector__msg__DummyCustomHeaderMessage * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    libstatistics_collector__msg__DummyCustomHeaderMessage__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence__init(libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  libstatistics_collector__msg__DummyCustomHeaderMessage * data = NULL;

  if (size) {
    data = (libstatistics_collector__msg__DummyCustomHeaderMessage *)allocator.zero_allocate(size, sizeof(libstatistics_collector__msg__DummyCustomHeaderMessage), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = libstatistics_collector__msg__DummyCustomHeaderMessage__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        libstatistics_collector__msg__DummyCustomHeaderMessage__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence__fini(libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      libstatistics_collector__msg__DummyCustomHeaderMessage__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence *
libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence * array = (libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence *)allocator.allocate(sizeof(libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence__destroy(libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence__are_equal(const libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence * lhs, const libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!libstatistics_collector__msg__DummyCustomHeaderMessage__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence__copy(
  const libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence * input,
  libstatistics_collector__msg__DummyCustomHeaderMessage__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(libstatistics_collector__msg__DummyCustomHeaderMessage);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    libstatistics_collector__msg__DummyCustomHeaderMessage * data =
      (libstatistics_collector__msg__DummyCustomHeaderMessage *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!libstatistics_collector__msg__DummyCustomHeaderMessage__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          libstatistics_collector__msg__DummyCustomHeaderMessage__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!libstatistics_collector__msg__DummyCustomHeaderMessage__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
