#include "base/base_inc.h"
#include "base/base_inc.c"

#include "render/render_inc.h"
#include "render/render_inc.c"

#include "font_provider/font_provider_inc.h"
#include "font_provider/font_provider_inc.c"

#include "font_cache/font_cache.h"
#include "font_cache/font_cache.c"

#include "vared.h"

#include <stdio.h>

internal Line line_alloc(Arena *arena, u32 size) {
    Line line = {0};
    line.buf = push_array(arena, u8, size);
    line.count = 0;
    line.size = size;
    return line;
}

internal void shift_at_cursor(Line *line, u32 cursor, int n) {
    assert(line->size >= line->count + n);
    assert(cursor <= line->count);
    assert(cursor + n < line->size);
    assert((i64)cursor + n >= 0);

    u8 *src = line->buf + cursor;
    u8 *dst = src + n;
    u64 count = line->count - cursor; 
    mem_move(dst, src, count);
    line->count += n;
}

internal void insert_char(Line *line, u32 *cursor, char c) {
    shift_at_cursor(line, *cursor, 1);
    line->buf[*cursor] = c;
    *cursor = *cursor + 1;
}

internal Line *new_line(Arena *arena, LineBuffer *text, u32 at) {
    assert(text->count < text->size);
    for (u32 i = text->count; i > at; i--) {
        text->lines[i] = text->lines[i - 1];
    }

    text->lines[at] = line_alloc(arena, kilobytes(1));
    text->count++;

    return &text->lines[at];
}

// TODO(fede): 
//      - Get font metrics, and do the vertical alignment with it.
//      - Render cursor.
//      - Handle emojis
//      - Handle newlines and such
internal v2 render_unicode_line_in_text_window(
        Line *line,
        Rect2 text_window,
        v2 text_offset) {

    // NOTE(fede): Assume text_offset sets the baseline
    v2 text_origin = v2_add(text_window.min, text_offset);

    v2 text_pos  = {0};

    String8 line_str = str8(line->buf, line->count);
    while (line_str.size) {
        UnicodeCodepoint codepoint = utf8_decode(line_str.str, line_str.size);
        assert(codepoint.byte_size <= 4);

        line_str = str8_skip(line_str, codepoint.byte_size);

        // TODO(fede): 
        //      - Add support for multiple textures.
        //      - Render an entire line at once (Font Run)? 
        FC_Glyph *glyph = fc_get_codepoint_glyph(codepoint.character, 40);
        
        {
            v2 pos = v2_add(text_origin, text_pos);
            pos = v2_add(pos, (v2){
                glyph->metrics.bearing_x,
                - glyph->metrics.bearing_y, // check if neg or pos
            });
            v2 dim = {
                glyph->metrics.width,
                glyph->metrics.height,
            };

            Rect2 glyph_pos = rect2_min_dim(pos, dim);

            r_push_rect2(glyph->tex, glyph_pos, glyph->uvs, (v4){1, 1, 1, 1});
        }

        text_pos.x += glyph->metrics.advance; 
    }

    text_offset.y += 50;

    return text_offset;
}

internal void editor_init(EditorParams *params) {
    // TODO(fede): change to *params->memory = arena_bootstrap_struct(EditorState, arena)
    // STUDY(fede): change commit/reserve sizes for this
    Arena *arena = arena_alloc();
    EditorState *state = push_struct(arena, EditorState);
    state->arena = arena;
    *params->memory = state;

    // STUDY(fede): change commit/reserve sizes for this
    state->frame_arena = arena_alloc();

    fc_init();
    fp_init();
    fp_open_font("fonts/NotoSans/static/NotoSans_Condensed-Black.ttf");

    // TODO(fede): Actual text data strutcture
    state->text.size = 1000;
    state->text.lines = push_array(state->arena, Line, state->text.size);
    state->text.count = 0;
    new_line(state->arena, &state->text, 0);

    {
        Line *line = &state->text.lines[state->cursor_line];
        insert_char(line, &state->cursor_char, 'h');
    }
}

void editor_update_and_render(EditorParams *params) {
    bool clear_frame_arena = true;
    if (!*params->memory) {
        editor_init(params);
        clear_frame_arena = false;
    }

    fc_tick();

    EditorState *state = (EditorState *)*params->memory;
    Arena *frame_arena = state->frame_arena;

    if (clear_frame_arena)
        arena_clear(frame_arena);

    state->text_window = rect2_center_dim(
            (v2){r_state->window_width / 2,  r_state->window_height / 2},
            (v2){r_state->window_width - 20, r_state->window_height - 20});

    WMEventList *events = params->events;
    for (; events->first; QueuePop(events->first, events->last)) {
        WMEvent event = events->first->v;
        switch (event.key) {

        case WMKey_RETURN: {
            new_line(state->arena, &state->text, state->cursor_line + 1);
            state->cursor_line++;
            state->cursor_char = 0;
        } break;

        // TODO(fede): UTF8 movement, for now this just moves per-byte instead
        //      of per-codepoint.
        case WMKey_BACKSPACE: {
            Line *line = &state->text.lines[state->cursor_line];
            u32 n = event.repeat == 0 ? 1 : event.repeat;
            n = min(state->cursor_char, n);
            shift_at_cursor(line, state->cursor_char, -(int)n);
            state->cursor_char -= n;
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

        case WMKey_o: {
            if ((event.modifiers & WMModifier_ctrl) && 
                !(event.modifiers & WMModifier_shift) &&
                !(event.modifiers & WMModifier_alt)) {
                Line line = state->text.lines[state->cursor_line];
                String8 path = str8(line.buf, line.count);

                // TODO(fede): Use scratch arena and implement pop
                char *path_cstr = cstr_from_str8(frame_arena, path);
                DebugReadFileResult file = debug_platform_read_entire_file(0, path_cstr);

                if (!file.memory) {
                    printf("Could not open file: %s\n", path_cstr);
                    break;
                }

                Line *new = new_line(state->arena, &state->text, state->cursor_line + 1);
                state->cursor_line++;
                state->cursor_char = 0;

                u32 bytes_to_copy = min(file.size, new->size);
                mem_copy(new->buf, file.memory, bytes_to_copy);
                new->count = bytes_to_copy;
            }
        } break;

        case WMKey_l: {
            if ((event.modifiers & WMModifier_ctrl) && 
                !(event.modifiers & WMModifier_shift) &&
                !(event.modifiers & WMModifier_alt)) {
                Line line = state->text.lines[state->cursor_line];
                String8 line_str = str8(line.buf, line.count);
                char *line_cstr = cstr_from_str8(frame_arena, line_str);
                printf("%s\n", line_cstr);
            }
        } break;

        default: {
            if (event.character) {
                Line *line = &state->text.lines[state->cursor_line];
                u8 insert_chars[4] = {0};
                u32 codepoint_byte_size = utf8_encode(event.character, (u8 *)insert_chars);
                for (u32 byte_idx = 0;
                        byte_idx < codepoint_byte_size;
                        byte_idx++) {
                    insert_char(line, &state->cursor_char, insert_chars[byte_idx]);
                }
            }
        } break; 
        }
    }

    r_push_rect2(nil_texture, state->text_window, (Rect2){0}, (v4){0.2, 0.2, 0.2, 1});

    v2 text_offset = {0, 50};
    for (u32 line_idx = 0; line_idx < state->text.count; line_idx++) {
        u32 *cursor_char = state->cursor_line == line_idx ? &state->cursor_char : 0;
        text_offset = render_unicode_line_in_text_window(
                &state->text.lines[line_idx],
                state->text_window,
                text_offset);
    }
}
