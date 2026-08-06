#include "base/base_inc.h"
#include "base/base_inc.c"

#include "render/render_inc.h"
#include "render/render_inc.c"

#include "font_provider/font_provider_inc.h"
#include "font_provider/font_provider_inc.c"

#include "font_cache/font_cache.h"
#include "font_cache/font_cache.c"

#include "ui/ui.h"
#include "ui/ui.c"

#include "vared.h"

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
    // TODO(fede): Real text data structure, this assert fires when opening a large 
    //      file
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
//      - Handle newlines and such
//      - Kerning
//      - Character aligning / hinting: STUDY
internal v2 render_unicode_line_in_text_window(
        FP_FontHandle font,
        f32 font_size,
        Line *line,
        Rect2 text_window,
        v2 text_offset,
        u32 *cursor_char,
        v2 *cursor_pos) {

    FP_FontMetrics metrics = fp_get_font_metrics(font, font_size);

    v2 text_origin = v2_add(text_window.min, text_offset);

    text_origin.y += metrics.ascender;

    v2 text_pos  = {0};

    bool cursor_pos_found = false;
    u32 byte_count = 0; 

    String8 line_str = str8(line->buf, line->count);
    while (line_str.size) {
        if (cursor_char && *cursor_char == byte_count) {
            *cursor_pos = v2_add(text_origin, text_pos);
            cursor_pos_found = true;
        }

        UnicodeCodepoint codepoint = utf8_decode(line_str.str, line_str.size);
        assert(codepoint.byte_size <= 4);

        line_str = str8_skip(line_str, codepoint.byte_size);
        byte_count += codepoint.byte_size;

        // TODO(fede): 
        //      - Render an entire line at once (Font Run)? 
        FC_Glyph *glyph = fc_get_codepoint_glyph(font, codepoint.character, font_size);
        
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

            r_push_rect2(.tex = glyph->tex, .pos = glyph_pos, .uv = glyph->uvs);
        }

        text_pos.x += glyph->metrics.advance; 
    }

    if (cursor_char && !cursor_pos_found) {
        assert(*cursor_char == byte_count);
        *cursor_pos = v2_add(text_origin, text_pos);
    }

    text_offset.y += metrics.height;

    return text_offset;
}

void editor_init(EditorParams *params) {
    // STUDY(fede): change to *params->memory = arena_bootstrap_struct(EditorState, arena)
    // STUDY(fede): change commit/reserve sizes for this
    Arena *arena = arena_alloc();
    EditorState *state = push_struct(arena, EditorState);
    state->arena = arena;
    *params->memory = state;

    // STUDY(fede): change commit/reserve sizes for this
    state->frame_arena = arena_alloc();

    fc_init();
    fp_init();
    // state->font = fp_open_font("fonts/NotoSans/static/NotoSans_Condensed-Black.ttf");
    state->font = fp_open_font("fonts/IosevkaTermNerdFontMono-Light.ttf");
    state->font_size = 15;

    ui_init();

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
    EditorState *state = (EditorState *)*params->memory;
    Arena *frame_arena = state->frame_arena;

    arena_clear(frame_arena);
    fc_tick();

    state->text_window = rect2_center_dim(
            (v2){r_state->window_width / 2,  r_state->window_height / 2},
            (v2){r_state->window_width - 20, r_state->window_height - 20});

    WMEventList *events = params->events;

    for (WMEventNode *event_n = events->first; event_n; event_n = event_n->next) {
        WMEvent event = event_n->v;
        switch (event.kind) {
        case WMEventKind_Press:
        case WMEventKind_Release: {
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
                // TODO(fede): Cleanup
                for (u32 i = 0; i < n;) {
                    u32 character_byte_length = 0;
                    do {
                        character_byte_length++;
                        i++;
                    } while (state->cursor_char > character_byte_length &&
                            !utf8_byte_is_header(line->buf[state->cursor_char - character_byte_length]));

                    shift_at_cursor(line, state->cursor_char, -(int)character_byte_length);
                    state->cursor_char -= character_byte_length;
                }
            } break;

            case WMKey_LEFT: {
                Line *line = &state->text.lines[state->cursor_line];
                do {
                    if (state->cursor_char > 0) 
                        state->cursor_char--;
                    else break;
                } while (!utf8_byte_is_header(line->buf[state->cursor_char]));
                // if (state->cursor_char > 0) 
                //     state->cursor_char--;
            } break;
            case WMKey_RIGHT: {
                Line *line = &state->text.lines[state->cursor_line];
                do {
                    if (state->cursor_char < line->count) 
                        state->cursor_char++;
                    else break;
                } while (!utf8_byte_is_header(line->buf[state->cursor_char]));
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

                    u32 lines_added = 0;
                    u8 *c = (u8 *)file.memory;
                    u32 line_start = 0;
                    for (u32 i = 0;; c++, i++) {
                        if (*c == '\n' || *c == 0) {
                            Line *new = new_line(
                                    state->arena,
                                    &state->text,
                                    state->cursor_line + 1 + lines_added);

                            u32 bytes_to_copy = i - line_start;
                            bytes_to_copy = min(bytes_to_copy, new->size);
                            u8 *src = (u8 *)file.memory + line_start;
                            mem_copy(new->buf, src, bytes_to_copy);
                            new->count = bytes_to_copy;

                            line_start = i + 1;
                            lines_added++;
                        }

                        if (*c == 0)
                            break;
                    }

                    debug_platform_free_file_memory(0, file);
                }
            } break;

            // TODO(fede): Fix this input, this never comes through, instead it 
            //      comes as (=, SHIFT). I do not know how to fix this yet. 
            case WMKey_PLUS: {
                if ((event.modifiers & WMModifier_ctrl) && 
                    !(event.modifiers & WMModifier_shift) && 
                    !(event.modifiers & WMModifier_alt)) {
                    state->font_size += 1;
                }
            } break;

            case WMKey_MINUS: {
                if ((event.modifiers & WMModifier_ctrl) && 
                    !(event.modifiers & WMModifier_alt)) {
                    if (event.modifiers & WMModifier_shift) {
                        state->font_size += 1;
                    } else {
                        state->font_size = max(1, state->font_size - 1);
                    }
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
        } break;

        case WMEventKind_MouseMove: {
            // TODO
            // printf("Mouse move: (%f, %f)\n", event.pos.x, event.pos.y);
            
        } break;
        }
    }

    v2 window_dim = {
        .x = r_state->window_width,
        .y = r_state->window_height,
    };
    ui_begin_build(window_dim, events);
    {
        ui_push_pref_width(ui_pct(0.75));
        ui_push_pref_height(ui_px(200));

        if (ui_button(S8("Button 1")).clicked) 
            printf("clicked 1!\n");
        if (ui_button(S8("Button 2")).clicked)
            printf("clicked 2!\n");

        ui_pop_pref_height();
        ui_pop_pref_width();
    }
    ui_end_build();

    ui_layout();
    ui_render();

    events->first = events->last = 0;

#if 0
    r_push_rect2(.pos = state->text_window, .color = (v4){0.2, 0.2, 0.2, 1});

#if 0
    {
        FP_FontMetrics metrics = fp_get_font_metrics(state->font, state->font_size);

        f32 height = state->text_window.min.y;
        while (height < state->text_window.max.y) {
            r_push_rect2(nil_texture, 
                    rect2_min_max( 
                        (v2){ state->text_window.min.x, height - 1 },
                        (v2){ state->text_window.max.x, height + 1 }),
                    (Rect2){0}, (v4){0.8, 0.8, 0.8, 1});

            r_push_rect2( 
                    .pos = rect2_min_max( 
                        (v2){ state->text_window.min.x, height + metrics.ascender - 1 },
                        (v2){ state->text_window.max.x, height + metrics.ascender + 1 }),
                    .color = (v4){0.8, 0.0, 0.0, 1});
            height += metrics.height;
        }
    }
#endif

#if 0
    {
        FP_FontMetrics metrics = fp_get_font_metrics(state->font, state->font_size);

        f32 height = state->text_window.min.y;
        while (height < state->text_window.max.y) {
            r_push_rect2(nil_texture, 
                    rect2_min_max( 
                        (v2){ state->text_window.min.x, height },
                        (v2){ state->text_window.max.x, height + metrics.height }),
                    (Rect2){0}, (v4){1, 1, 1, 0.1});
            height += metrics.height * 2;
        }
    }
#endif

    {
        FP_FontMetrics metrics = fp_get_font_metrics(state->font, state->font_size);

        v2 text_offset = {0};
        v2 cursor_pos = {0};
        for (u32 line_idx = 0;
                line_idx < state->text.count &&
                text_offset.y + metrics.height <= state->text_window.max.y;
                line_idx++) {
            u32 *cursor_char = state->cursor_line == line_idx ? &state->cursor_char : 0;
            text_offset = render_unicode_line_in_text_window(
                    state->font,
                    state->font_size,
                    &state->text.lines[line_idx],
                    state->text_window,
                    text_offset, 
                    cursor_char,
                    &cursor_pos);
        }


        cursor_pos.y -= metrics.ascender;
        Rect2 cursor_rect = rect2_min_dim(cursor_pos, (v2){ metrics.height / 4, metrics.height });
        r_push_rect2(.pos = cursor_rect, .color = (v4){1, 1, 1, 0.5});
    }

    r_push_rect2(
            .pos = rect2_min_dim((v2){300, 300}, (v2){200, 200}),
            .corner_radius = 60,
            .edge_softness = 10);
#endif
}
