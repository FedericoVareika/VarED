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

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

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
    u64 count;
    u64 size;
    u8 *buf;
} Line;

typedef struct {
    Line *lines; 
    u32 count;
    u32 size;
} LineBuffer;

typedef struct {
    Arena *arena;
    Arena *frame_arena;

    Font font;

    Rect2 text_window;
    LineBuffer text;
    u32 cursor_line;
    u32 cursor_char;
} EditorState;

#endif // VARED_H
