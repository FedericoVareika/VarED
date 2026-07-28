#include "base/base_inc.h"
#include "base/base_inc.c"

#include "vared.h"

#include "vared_renderer.h"
#include "vared_renderer.c"

#include <stdio.h>

internal S8 s8_alloc(Arena *arena, u32 size) {
    S8 string = {0};
    string.buf = push_array(arena, u8, size);
    string.count = 0;
    string.size = size;
    return string;
}

internal void shift_at_cursor(S8 *text, u32 cursor, int n) {
    assert(text->size >= text->count + n);
    assert(cursor <= text->count);
    assert(cursor + n < text->size);
    assert((i64)cursor + n > 0);

    u8 *src = text->buf + cursor;
    u8 *dst = src + n;
    u64 count = text->count - cursor; 
    mem_move(dst, src, count);
    text->count += n;
}

internal void insert_char(S8 *string, u32 *cursor, char c) {
    shift_at_cursor(string, *cursor, 1);
    string->buf[*cursor] = c;
    *cursor = *cursor + 1;
}

internal void new_line(Arena *arena, LineBuffer *text, u32 at) {
    assert(text->count < text->size);
    for (u32 i = text->count; i > at; i--) {
        text->lines[i] = text->lines[i - 1];
    }

    text->lines[at] = s8_alloc(arena, kilobytes(1));
    text->count++;
}

internal void render_s8_in_rect2(
        EditorState *state,
        RenderGroup *render_group,
        S8 string,
        u32 *cursor,
        Rect2 *window_rect,
        v4 color) {
    Font *font = &state->font;

    rg_push_set_shader(state->frame_arena, render_group, ShaderType_Text);

    f32 scale = font->font_height / (font->ascent - font->descent);

    stbtt_aligned_quad q;

    f32 current_x = window_rect->min.x;
    f32 current_y = window_rect->min.y; 
    current_y += scale * (f32)font->ascent;

    v2 cursor_pos = { current_x, current_y };

    char *c = (char *)string.buf;
    for (u32 string_idx = 0;
            string_idx < string.count; 
            string_idx++, c++) {
        int advance_width, left_side_bearing; 
        stbtt_GetCodepointHMetrics(&font->font_info, *c, &advance_width, &left_side_bearing);

        if (string_idx == 0) {
            current_x += scale * left_side_bearing;
        }

        if (current_x + scale * advance_width > window_rect->max.x) {
            current_x = window_rect->min.x + scale * left_side_bearing;
            current_y += scale * (f32)(font->y_increment);
            window_rect->min.y += scale;
        }

        stbtt_GetPackedQuad(
                font->char_data_for_range,
                font->pixels_width, font->pixels_height,
                *c - font->first_char, 
                &current_x, &current_y, &q,
                true);

        if (current_y - scale * font->descent > window_rect->max.y)
            break;

        rg_push_textured_rect2(
                state->frame_arena,
                render_group, 
                rect2_min_max((v2){q.x0, q.y0}, (v2){q.x1, q.y1}),
                color,
                rect2_min_max((v2){q.s0, q.t0}, (v2){q.s1, q.t1}));

        if (string_idx < string.count) {
            current_x += scale * (f32)stbtt_GetCodepointKernAdvance(
                    &font->font_info, *c, *(c + 1));
        }
        
        if (cursor && string_idx + 1 == *cursor) {
            cursor_pos = (v2){ current_x, current_y };
        }

    }

    window_rect->min.y += scale * (f32)(font->y_increment);

    if (cursor) {
        rg_push_set_shader(
                state->frame_arena,
                render_group, ShaderType_Color);

        f32 cursor_width = 2;
        v2 cursor_upper_left = v2_sub(cursor_pos, (v2){cursor_width / 2, scale * font->ascent});

        rg_push_rect2(
                state->frame_arena,
                render_group, 
                rect2_min_dim(cursor_upper_left, (v2){cursor_width, font->font_height}),
                color);
    }
}

internal void editor_init(EditorParams *params, RenderGroup *render_group) {
    // TODO(fede): change to *params->memory = arena_bootstrap_struct(EditorState, arena)
    // STUDY(fede): change commit/reserve sizes for this
    Arena *arena = arena_alloc();
    EditorState *state = push_struct(arena, EditorState);
    state->arena = arena;
    *params->memory = state;

    // STUDY(fede): change commit/reserve sizes for this
    state->frame_arena = arena_alloc();

    PlatformApi platform = params->platform;

    Font *font = &state->font;
    {
        font->pixels_width = 512;
        font->pixels_height = 512;
        font->first_char = 32;
        font->num_chars = 96;
        font->font_height = 32; // NOTE(fede): pixels i think
        stbtt_fontinfo *font_info = &font->font_info;

        DebugReadFileResult font_file = platform.debug_platform_read_entire_file(
                0, "fonts/Roboto/static/Roboto-Medium.ttf");
        // 0, "fonts/IosevkaTermNerdFontMono-Light.ttf");
        // 0, "fonts/IosevkaTermNerdFont-Light.ttf");

        assert(stbtt_InitFont(font_info, font_file.memory, 0));

        u8 *pixels = push_array(state->frame_arena, u8, 
                font->pixels_width * font->pixels_height);

        stbtt_pack_context spc;
        assert(stbtt_PackBegin(&spc,
                    pixels,
                    font->pixels_width, font->pixels_height,
                    0, 1,
                    0));

        font->char_data_for_range = push_array(
                state->arena, stbtt_packedchar, font->num_chars);
        assert(stbtt_PackFontRange(&spc,
                    font_file.memory,
                    0, font->font_height, 
                    font->first_char, font->num_chars,
                    font->char_data_for_range));

        stbtt_PackEnd(&spc);

        stbtt_GetFontVMetrics(&font->font_info,
                &font->ascent, &font->descent, &font->line_gap);
        font->y_increment = font->ascent - font->descent + font->line_gap;

        rg_push_texture_load(
                state->frame_arena,
                render_group, pixels,
                font->pixels_width, font->pixels_height);
    }

    // TODO(fede): Actual text data strutcture
    state->text.size = 1000;
    state->text.lines = push_array(state->arena, S8, state->text.size);
    state->text.count = 0;
    new_line(state->arena, &state->text, 0);
}

extern EDITOR_UPDATE_AND_RENDER(editor_update_and_render) {
    bool clear_frame_arena = true;
    if (!*params->memory) {
        editor_init(params, render_group);
        clear_frame_arena = false;
    }

    EditorState *state = (EditorState *)*params->memory;
    Arena *frame_arena = state->frame_arena;

    if (clear_frame_arena)
        arena_clear(frame_arena);

    state->text_window = rect2_center_dim(
            (v2){render_group->width / 2, render_group->height / 2},
            (v2){500, 6 * state->font.font_height});

    Font font = state->font;
    int min_key = font.first_char;
    int max_key = font.first_char + font.num_chars;

    WMEventList *events = params->events;
    for (; events->first; QueuePop(events->first, events->last)) {
        WMEvent event = events->first->v;
        switch (event.key) {
        case WMKey_NONE: 
            break;

        case WMKey_RETURN: {
            new_line(state->arena, &state->text, state->cursor_line + 1);
            state->cursor_line++;
            state->cursor_char = 0;
        } break;
        case WMKey_LEFT: {
            if (state->cursor_char > 0) 
                state->cursor_char--;
        } break;
        case WMKey_RIGHT: {
            if (state->cursor_char < 
                    state->text.lines[state->cursor_line].count) 
                state->cursor_char++;
        } break;
        case WMKey_UP: {
            if (state->cursor_line > 0) {
                state->cursor_line--;
                state->cursor_char = 0;
            }
        } break;
        case WMKey_DOWN: {
            if (state->cursor_line + 1 < 
                    state->text.count) {
                state->cursor_line++;
                state->cursor_char = 0;
            }
        } break;

        default: {
            S8 *line = &state->text.lines[state->cursor_line];
            u8 c = (u8)event.character;
            assert((u32)c == event.character); // NOTE(fede): Only ASCII
            insert_char(line, &state->cursor_char, c);
        } break; 
        }
    }
#if 0
    for (u32 i = 0; i < input->text_input_count; i++) {
        TextInput text_input = input->text_inputs[i];
        for (u32 j = 0; j < text_input.text_len; j++) {
            S8 *line = &state->text.lines[state->cursor_line];
            insert_char(line, &state->cursor_char, text_input.text[j]);
        }
    }

    for (u32 i = 0; i < input->move_input_count; i++) {
        KeyboardInputType move_type = input->move_inputs[i];
        switch (move_type) {
        case KeyboardInputType_Left: 
            if (state->cursor_char > 0) 
                state->cursor_char--;
            break;
        case KeyboardInputType_Right: 
            if (state->cursor_char < 
                    state->text.lines[state->cursor_line].count) 
                state->cursor_char++;
            break;
        case KeyboardInputType_Up: 
            if (state->cursor_line > 0) {
                state->cursor_line--;
                state->cursor_char = 0;
            }
            break;
        case KeyboardInputType_Down: 
            if (state->cursor_line + 1 < 
                    state->text.count) {
                state->cursor_line++;
                state->cursor_char = 0;
            }
            break;

        case KeyboardInputType_Return: 
            new_line(&state->arena, &state->text, state->cursor_line + 1);
            state->cursor_line++;
            state->cursor_char = 0;
            break;
        default:
            break;
        }
    }
#endif 

    rg_push_set_shader(frame_arena, render_group, ShaderType_Color);
    rg_push_rect2(frame_arena, render_group, state->text_window, (v4){0.2, 0.2, 0.2, 1});

    Rect2 line_window = state->text_window;
    for (u32 line_idx = 0; line_idx < state->text.count; line_idx++) {
        u32 *cursor_char = state->cursor_line == line_idx ? &state->cursor_char : 0;
        render_s8_in_rect2(
                state, 
                render_group,
                state->text.lines[line_idx],
                cursor_char,
                &line_window,
                (v4){0.8, 0.8, 0.8, 1});
    }
}
