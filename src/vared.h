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

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

typedef struct {
    stbtt_fontinfo font_info;
    stbtt_packedchar *char_data_for_range;

    u32 pixels_width;
    u32 pixels_height;
    u32 first_char;
    u32 num_chars;
    f32 font_height;

    f32 scale;
    int ascent;
    int descent;
    int line_gap;

    int y_increment;
} Font;

typedef struct {
    u8 *buf;
    u32 count;
    u32 size;
} S8;

typedef struct {
    Arena arena;

    Font font;

    Rect2 text_window;
    S8 string;
} EditorState;

#endif // VARED_H
