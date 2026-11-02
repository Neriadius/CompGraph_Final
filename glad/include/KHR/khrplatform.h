#ifndef __khrplatform_h_
#define __khrplatform_h_
/* Minimal Khronos platform header (type definitions used by glad). */
#if defined(_WIN32) && !defined(__SCITECH_SNAP__)
#   define KHRONOS_APICALL __declspec(dllimport)
#elif defined(__SYMBIAN32__)
#   define KHRONOS_APICALL IMPORT_C
#elif defined(__ANDROID__)
#   define KHRONOS_APICALL __attribute__((visibility("default")))
#else
#   define KHRONOS_APICALL
#endif

#if defined(_WIN32) && !defined(_WIN64) && !defined(__SCITECH_SNAP__)
#   define KHRONOS_APIENTRY __stdcall
#else
#   define KHRONOS_APIENTRY
#endif
#define KHRONOS_APIATTRIBUTES

#include <stdint.h>
typedef int32_t  khronos_int32_t;
typedef uint32_t khronos_uint32_t;
typedef int64_t  khronos_int64_t;
typedef uint64_t khronos_uint64_t;
#define KHRONOS_SUPPORT_INT64 1
#define KHRONOS_SUPPORT_FLOAT 1
typedef signed   char  khronos_int8_t;
typedef unsigned char  khronos_uint8_t;
typedef signed   short khronos_int16_t;
typedef unsigned short khronos_uint16_t;
typedef intptr_t  khronos_intptr_t;
typedef uintptr_t khronos_uintptr_t;
typedef intptr_t  khronos_ssize_t;
typedef uintptr_t khronos_usize_t;
typedef float khronos_float_t;
typedef khronos_uint64_t khronos_utime_nanoseconds_t;
typedef khronos_int64_t  khronos_stime_nanoseconds_t;
#define KHRONOS_MAX_ENUM 0x7FFFFFFF
typedef enum { KHRONOS_FALSE = 0, KHRONOS_TRUE = 1, KHRONOS_BOOLEAN_ENUM_FORCE_SIZE = KHRONOS_MAX_ENUM } khronos_boolean_enum_t;
#endif
