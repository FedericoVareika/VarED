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

    // NOTE(fede): 
    //      A focused view is one where we are editing the text in it,
    //      whereas a selected file view is the tab that is currently selected, 
    //      this one should always be rendered, but interactions should be 
    //      done on the focused view.
    TXT_ViewNode *focused_view;
    TXT_ViewNode *selected_file_view;

    f32 font_size;
    FP_Handle font;

    f32 line_height;

    bool is_opening_file;
    bool show_profiler;
} EditorState;

#endif // VARED_H
