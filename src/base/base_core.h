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
#define abs(a) ((a) < 0 ? -(a) : (a))

#define array_count(a) (sizeof((a)) / sizeof((a)[0]))

#define IsNil(x, nil) (x) == 0 || (x) == (nil)

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Queue

#define QueuePush_N_nil(f, l, n, next, nil) IsNil(f, nil) ? \
    ((f) = (l) = (n), (n)->next = (nil)) : \
    ((l)->next = (n), (l) = (n), (n)->next = (nil))

#define QueuePop_N_nil(f, l, next, nil) ((f) == (l)) ? \
    ((f) = (l) = (nil)) : \
    ((f) = (f)->next)

#define QueuePush_N(f, l, n, next) QueuePush_N_nil(f, l, n, next, 0)
#define QueuePop_N(f, l, next) QueuePop_N_nil(f, l, next, 0)
#define QueuePush(f, l, n) QueuePush_N(f, l, n, next)
#define QueuePop(f, l) QueuePop_N(f, l, next)
#define QueuePush_nil(f, l, n, nil) QueuePush_N_nil(f, l, n, next, nil)
#define QueuePop_nil(f, l, nil) QueuePop_N_nil(f, l, next, nil)

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): DLL (Doubly-Linked-List)

#define DLL_Insert_NP_nil(f, l, p, n, next, prev, nil) IsNil(f, nil) ? \
    ((f) = (l) = (n), (n)->next = (nil), (n)->prev = (nil)) : \
    (IsNil(p, nil) ? \
      ((n)->next = (f), (n)->prev = (nil), (f) = (n)) : \
      ((p) == (l) ? (0) : ((p)->next->prev = (n)), (n)->prev = (p), (n)->next = (p)->next, (p)->next = (n)))

#define DLL_PushBack_NP_nil(f, l, n, next, prev, nil) DLL_Insert_NP_nil(f, l, l, n, next, prev, nil)
#define DLL_PushBack_nil(f, l, n, nil) DLL_PushBack_NP_nil(f, l, n, next, prev, nil)
#define DLL_PushBack(f, l, n) DLL_PushBack_NP_nil(f, l, n, next, prev, 0)

#define DLL_PushFront_NP_nil(f, l, n, next, prev, nil) DLL_Insert_NP_nil(l, f, f, n, prev, next, nil)
#define DLL_PushFront_nil(f, l, n, nil) DLL_PushFront_NP_nil(f, l, n, next, prev, nil)
#define DLL_PushFront(f, l, n) DLL_PushFront_NP_nil(f, l, n, next, prev, 0)

#define DLL_Remove_NP_nil(f, l, n, next, prev, nil) ((f) == (l)) ? \
    ((f) = (l) = (nil)) : \
    ((n) == (f) ? \
     ((f) = (f)->next, (f)->prev = (nil)) : \
     ((n) == (l) ? \
      ((l) = (l)->prev, (l)->next = (nil)) : \
      ((n)->prev->next = (n)->next)))

#define DLL_Remove_nil(f, l, n, nil) DLL_Remove_NP_nil(f, l, n, next, prev, nil)
#define DLL_Remove(f, l, n) DLL_Remove_nil(f, l, n, nil)

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): SLL (Singly-Linked-List)

#define SLL_Insert_N_nil(f, l, p, n, next, nil) IsNil(f, nil) ? \
    ((f) = (l) = (n), (n)->next = (nil)) : \
    (IsNil(p, nil) ? \
     ((n)->next = (f), (f) = (n)) : \
     ((n)->next = (p)->next, (p)->next = (n), (p) == (l) ? \
      ((l) = (n)) : (0)))

#define SLL_PushBack_N_nil(f, l, n, next, nil) SLL_Insert_N_nil(f, l, l, n, next, nil)
#define SLL_PushBack_nil(f, l, n, nil) SLL_PushBack_N_nil(f, l, n, next, nil)
#define SLL_PushBack(f, l, n) SLL_PushBack_nil(f, l, n, 0)

#endif // BASE_CORE_H
