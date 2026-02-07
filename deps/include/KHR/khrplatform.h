#ifndef __khrplatform_h_
#define __khrplatform_h_

#include <stdint.h>

typedef int32_t  khronos_int32_t;
typedef uint32_t khronos_uint32_t;
typedef int64_t  khronos_int64_t;
typedef uint64_t khronos_uint64_t;
typedef int8_t   khronos_int8_t;
typedef uint8_t  khronos_uint8_t;
typedef int16_t  khronos_int16_t;
typedef uint16_t khronos_uint16_t;
typedef float    khronos_float_t;

#if defined(_WIN64)
typedef signed   long long int khronos_intptr_t;
typedef unsigned long long int khronos_uintptr_t;
typedef signed   long long int khronos_ssize_t;
typedef unsigned long long int khronos_usize_t;
#else
typedef signed   long  int khronos_intptr_t;
typedef unsigned long  int khronos_uintptr_t;
typedef signed   long  int khronos_ssize_t;
typedef unsigned long  int khronos_usize_t;
#endif

#endif