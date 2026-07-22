#ifndef VARED_H
#define VARED_H

/*
 * NOTE(fede):
 *
 *  VARED_SLOW:
 *    0 - No slow code allowed.
 *    1 - Slow code allowed.
 *
 *  VARED_INTERNAL:
 *    0 - Build for public use.
 *    1 - Build for developer only.
 *
 * */

#include "vared_platform.h"

#if VARED_SLOW

#define assert(expression)                                                     \
    if (!(expression)) {                                                       \
        *(int *)0 = 0;                                                         \
    }

#else

#define assert(expression)

#endif

#define PI 3.14159265359f

#define kilobytes(value) ((value) * 1024)
#define megabytes(value) (kilobytes(value) * 1024)
#define gigabytes(value) (megabytes(value) * 1024)
#define terabytes(value) (gigabytes(value) * 1024)

#include <stdlib.h>
#if 0
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define abs(a) ((a) < 0 ? -(a) : (a))

// TODO(fede): does not work
#define STBTT_fabs(abs); 
#define STBTT_max(abs); 
#define STBTT_min(min); 
#endif 


#define array_count(a) (sizeof((a)) / sizeof((a)[0]))

#include "vared_arena.c"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

typedef struct {
    int num_chars;
    f32 pixel_height;

    u32 bitmap_width;
    u32 bitmap_height;

    stbtt_bakedchar *cdata;
} Font;

typedef struct {
    Arena arena;

    Font font;
} EditorState;

#endif // VARED_H
