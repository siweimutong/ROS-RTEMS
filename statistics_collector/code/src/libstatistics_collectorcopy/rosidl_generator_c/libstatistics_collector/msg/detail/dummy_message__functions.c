// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from libstatistics_collector:msg/DummyMessage.idl
// generated code does not contain a copyright notice
#include "libstatistics_collector/msg/detail/dummy_message__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `header`
#include "std_msgs/msg/detail/header__functions.h"

bool
libstatistics_collector__msg__DummyMessage__init(libstatistics_collector__msg__DummyMessage * msg)
{
  if (!msg) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__init(&msg->header)) {
    libstatistics_collector__msg__DummyMessage__fini(msg);
    return false;
  }
  return true;
}

void
libstatistics_collector__msg__DummyMessage__fini(libstatistics_collector__msg__DummyMessage * msg)
{
  if (!msg) {
    return;
  }
  // header
  std_msgs__msg__Header__fini(&msg->header);
}

bool
libstatistics_collector__msg__DummyMessage__are_equal(const libstatistics_collector__msg__DummyMessage * lhs, const libstatistics_collector__msg__DummyMessage * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__are_equal(
      &(lhs->header), &(rhs->header)))
  {
    return false;
  }
  return true;
}

bool
libstatistics_collector__msg__DummyMessage__copy(
  const libstatistics_collector__msg__DummyMessage * input,
  libstatistics_collector__msg__DummyMessage * output)
{
  if (!input || !output) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__copy(
      &(input->header), &(output->header)))
  {
    return false;
  }
  return true;
}

libstatistics_collector__msg__DummyMessage *
libstatistics_collector__msg__DummyMessage__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  libstatistics_collector__msg__DummyMessage * msg = (libstatistics_collector__msg__DummyMessage *)allocator.allocate(sizeof(libstatistics_collector__msg__DummyMessage), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(libstatistics_collector__msg__DummyMessage));
  bool success = libstatistics_collector__msg__DummyMessage__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
libstatistics_collector__msg__DummyMessage__destroy(libstatistics_collector__msg__DummyMessage * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    libstatistics_collector__msg__DummyMessage__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
libstatistics_collector__msg__DummyMessage__Sequence__init(libstatistics_collector__msg__DummyMessage__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  libstatistics_collector__msg__DummyMessage * data = NULL;

  if (size) {
    data = (libstatistics_collector__msg__DummyMessage *)allocator.zero_allocate(size, sizeof(libstatistics_collector__msg__DummyMessage), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = libstatistics_collector__msg__DummyMessage__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        libstatistics_collector__msg__DummyMessage__fini(&data[i - 1]);
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
libstatistics_collector__msg__DummyMessage__Sequence__fini(libstatistics_collector__msg__DummyMessage__Sequence * array)
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
      libstatistics_collector__msg__DummyMessage__fini(&array->data[i]);
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

libstatistics_collector__msg__DummyMessage__Sequence *
libstatistics_collector__msg__DummyMessage__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  libstatistics_collector__msg__DummyMessage__Sequence * array = (libstatistics_collector__msg__DummyMessage__Sequence *)allocator.allocate(sizeof(libstatistics_collector__msg__DummyMessage__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = libstatistics_collector__msg__DummyMessage__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
libstatistics_collector__msg__DummyMessage__Sequence__destroy(libstatistics_collector__msg__DummyMessage__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    libstatistics_collector__msg__DummyMessage__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
libstatistics_collector__msg__DummyMessage__Sequence__are_equal(const libstatistics_collector__msg__DummyMessage__Sequence * lhs, const libstatistics_collector__msg__DummyMessage__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!libstatistics_collector__msg__DummyMessage__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
libstatistics_collector__msg__DummyMessage__Sequence__copy(
  const libstatistics_collector__msg__DummyMessage__Sequence * input,
  libstatistics_collector__msg__DummyMessage__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(libstatistics_collector__msg__DummyMessage);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    libstatistics_collector__msg__DummyMessage * data =
      (libstatistics_collector__msg__DummyMessage *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!libstatistics_collector__msg__DummyMessage__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          libstatistics_collector__msg__DummyMessage__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!libstatistics_collector__msg__DummyMessage__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
