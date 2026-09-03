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
#include "text/text_inc.h"

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

    TXT_ViewNode *input_view_n;

    TXT_ViewNode *first_view;
    TXT_ViewNode *last_view;

    TXT_ViewNode *main_selected_view_n;
    TXT_ViewNode *general_selected_view_n;

    f32 font_size;
    FP_Handle font;

    bool is_opening_file;
    bool show_profiler;
} EditorState;

#endif // VARED_H
