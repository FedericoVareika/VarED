#ifndef BASE_CORE_H
#define BASE_CORE_H

/*
 * NOTE(fede): Compilers
*/
#ifdef __GNUC__
    #ifndef __clang__
        #define COMPILER_GCC
    #else
        #define COMPILER_CLANG
    #endif //__clang__
#endif //__GNUC__

# if defined(_WIN32)
#  define OS_WINDOWS 1
# elif defined(__gnu_linux__) || defined(__linux__)
#  define OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# else
#  error This compiler/OS combo is not supported.
# endif

#include <stdint.h>
#include <stdio.h> // NOTE(fede): for size_t type

#define internal static
#define global static
#define local_persist static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

// STUDY(fede): This is added because of SDL, check if this is necessary.
#if !defined(__bool_true_false_are_defined)
typedef enum { false, true } bool;
#endif

typedef struct {
    int placeholder;
} ThreadContext;

#if VARED_SLOW

#define assert(expression)                                                     \
    if (!(expression)) {                                                       \
        *(int *)0 = 0;                                                         \
    }

#else

#define assert(expression) (expression)

#endif

#define PI 3.14159265359f

#define kilobytes(value) ((value) * 1024)
#define megabytes(value) (kilobytes(value) * 1024)
#define gigabytes(value) (megabytes(value) * 1024)
#define terabytes(value) (gigabytes(value) * 1024)

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#if 0 
#define abs(a) ((a) < 0 ? -(a) : (a))

#include <stdlib.h>
// TODO(fede): does not work
#define STBTT_fabs(abs);
#define STBTT_max(abs);
#define STBTT_min(min);
#endif

#define array_count(a) (sizeof((a)) / sizeof((a)[0]))

////////////////////////////////
// Queue

#define QueuePush_N(f, l, n, next) ((f) == 0) ? \
    ((f) = (l) = (n), (n)->next = 0) : \
    ((l)->next = (n), (l) = (n), (n)->next = 0)
#define QueuePop_N(f, l, next) ((f) == (l)) ? \
    ((f) = (l) = 0) : \
    ((f) = (f)->next)

#define QueuePush(f, l, n) QueuePush_N(f, l, n, next)
#define QueuePop(f, l) QueuePop_N(f, l, next)


#endif // BASE_CORE_H
