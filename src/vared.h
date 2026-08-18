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

#include "base/base_inc.h"
#include "render/render_inc.h"
#include "font_provider/font_provider_inc.h"
#include "text/text.h"

#include "vared_platform.h"

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

    f32 font_size;
    FP_Handle font;

    Rect2 text_window;
    LineBuffer text;
    u32 cursor_line;
    u32 cursor_char;

    Arena *text_arena;
    TXT_Text *text_;
    u32 cursor_row;
    u32 cursor_col;         // in characters (utf-8)
    u32 cursor_col_bytes;   // in bytes

    bool is_opening_file;

    bool bval;
    f32 val1;
    f32 val2;
} EditorState;

#endif // VARED_H
