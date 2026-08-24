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

#include "text/text.h"
#include "text/text.c"

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

void editor_init(EditorParams *params) {
    // STUDY(fede): change to *params->memory = arena_bootstrap_struct(EditorState, arena)
    // STUDY(fede): change commit/reserve sizes for this
    Arena *arena = arena_alloc();
    EditorState *state = push_struct(arena, EditorState);
    state->arena = arena;
    *params->memory = state;

    // STUDY(fede): change commit/reserve sizes for this
    state->frame_arena = arena_alloc();

    state->text_arena = arena_alloc();
    state->text = push_struct(state->text_arena, TXT_Text);

    fc_init();
    fp_init();
    state->font = fp_open_font("data/fonts/GoogleSans-Regular.ttf");
    state->font_size = 14;

    ui_init();
}

void editor_update_and_render(EditorParams *params) {
    EditorState *state = (EditorState *)*params->memory;
    Arena *frame_arena = state->frame_arena;

    arena_clear(frame_arena);
    fc_tick();

    WMEventList *events = params->events;

    for (WMEventNode *event_n = events->first; event_n; event_n = event_n->next) {
        WMEvent event = event_n->v;
        switch (event.kind) {
        case WMEventKind_Press:
        case WMEventKind_Release: {
            switch (event.key) {

            case WMKey_RETURN: {
                u64 at = txt_get_line_offset(state->text, state->cursor_row);
                at += state->cursor_col_bytes;
                txt_insert(state->text_arena, state->text, S8("\n"), at);
                state->cursor_col_bytes = 0;
                state->cursor_row++;
            } break;

            case WMKey_BACKSPACE: {
            } break;

            case WMKey_LEFT: {
                // TODO(fede): Scratch arena.
                String8 line = txt_get_line(state->frame_arena, state->text, state->cursor_row);
                do {
                    if (state->cursor_col_bytes > 0) {
                        state->cursor_col_bytes--;
                    } else {
                        if (state->cursor_row)
                            state->cursor_row--;

                        line = txt_get_line(state->frame_arena, state->text, state->cursor_row);
                        state->cursor_col_bytes = line.size - 1; // before the \n
                    }
                } while (!utf8_byte_is_header(line.str[state->cursor_col_bytes]));
            } break;
            case WMKey_RIGHT: {
                u32 new_cursor_col_bytes = state->cursor_col_bytes; 
                u32 new_cursor_row = state->cursor_row; 

                // TODO(fede): Scratch arena.
                String8 line = txt_get_line(state->frame_arena, state->text, state->cursor_row);
                
                if (new_cursor_col_bytes + 1 == line.size && line.str[new_cursor_col_bytes] == '\n')
                    new_cursor_col_bytes++;

                do {
                    if (new_cursor_col_bytes < line.size) {
                        new_cursor_col_bytes++;
                    } else if (new_cursor_col_bytes == line.size) {
                        if (txt_get_n_lines(state->text) <= new_cursor_row + 1) {
                            new_cursor_col_bytes = state->cursor_col_bytes;
                            new_cursor_row = state->cursor_row;
                            break;
                        }

                        new_cursor_row++;
                        line = txt_get_line(state->frame_arena, state->text, new_cursor_row);
                        new_cursor_col_bytes = 0;
                        break;
                    }
                } while (!utf8_byte_is_header(line.str[new_cursor_col_bytes]));

                state->cursor_col_bytes = new_cursor_col_bytes;
                state->cursor_row = new_cursor_row;
            } break;
            case WMKey_UP: {
                if (state->cursor_row > 0) {
                    state->cursor_row--;

                    // TODO(fede): Do optically aligned, instead of byte aligned.
                    String8 line = txt_get_line(state->frame_arena, state->text, state->cursor_row);
                    while (!utf8_byte_is_header(line.str[state->cursor_col_bytes])) {
                        state->cursor_col_bytes--;
                    }
                }
            } break;
            case WMKey_DOWN: {
                if (state->cursor_row + 1 < txt_get_n_lines(state->text)) {
                    state->cursor_row++;

                    // TODO(fede): Do optically aligned, instead of byte aligned.
                    String8 line = txt_get_line(state->frame_arena, state->text, state->cursor_row);
                    while (!utf8_byte_is_header(line.str[state->cursor_col_bytes])) {
                        state->cursor_col_bytes--;
                    }
                }
            } break;

            case WMKey_o: {
                if ((event.modifiers & WMModifier_ctrl) && 
                    !(event.modifiers & WMModifier_shift) &&
                    !(event.modifiers & WMModifier_alt)) {
                    // TODO(fede): Use scratch arena and implement pop
                    String8 path = txt_get_line(state->frame_arena, state->text, state->cursor_row);

                    // TODO(fede): Use scratch arena and implement pop
                    char *path_cstr = cstr_from_str8(frame_arena, path);
                    DebugReadFileResult file = debug_platform_read_entire_file(0, path_cstr);

                    if (!file.memory) {
                        printf("Could not open file: %s\n", path_cstr);
                        break;
                    }

                    u64 at = txt_get_line_offset(state->text, state->cursor_row);
                    at += state->cursor_col_bytes;
                    txt_insert(state->text_arena, state->text, str8(file.memory, file.size), at);

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

            default: {
                if (event.character) {
                    u8 insert_chars[4] = {0};
                    u32 codepoint_byte_size = utf8_encode(event.character, (u8 *)insert_chars);

                    u64 at = txt_get_line_offset(state->text, state->cursor_row);
                    at += state->cursor_col_bytes;
                    txt_insert(state->text_arena, state->text, str8((u8 *)&insert_chars, codepoint_byte_size), at);
                    state->cursor_col_bytes += codepoint_byte_size;
                }
            } break; 
            }
        } break;

        case WMEventKind_MouseMove: {
            
        } break;
        }
    }

    v2 window_dim = {
        .x = r_state->window_width,
        .y = r_state->window_height,
    };
    R_Bucket *main_bucket = r_get_new_bucket();
    r_push_bucket(main_bucket);

    ui_begin_build(window_dim, events, params->dt);

    UI_Font(state->font)
        UI_FontSize(state->font_size)
        UI_BackgroundColor(RGBA(0.11, 0.11, 0.11, 1))
        UI_BorderColor(RGBA(0, 0, 0, 0))
    {
        UI_Row
        UI_Parent(ui_box_makef(UI_BoxFlag_DrawBackground, ""))
            UI_PrefHeight(ui_pct(1, 0)) UI_PrefWidth(ui_pct(1, 0))
            UI_BorderColor(RGBA(0.4, 0.5, 0.5, 1))
        {
            UI_Row
                UI_Padding(ui_em(5, 0))
                UI_ChildLayoutAxis(UI_Axis2_Y)
                UI_PrefWidth(ui_pct(1, 0))
                UI_Parent(ui_box_makef(0, "panel 1"))
                UI_PrefWidth(ui_tc(10, 0)) UI_PrefHeight(ui_em(2, 1))
            {
                ui_button(S8("BRAVO"));
                if (ui_button(S8("Use Iosevka")).clicked) {
                    fp_close_font(state->font);
                    fc_flush();
                    state->font = fp_open_font("data/fonts/IosevkaTermNerdFontMono-Light.ttf");
                    printf("Using Iosevka\n");
                }
                if (ui_button(S8("Use Google Sans")).clicked) {
                    fp_close_font(state->font);
                    fc_flush();
                    state->font = fp_open_font("data/fonts/GoogleSans-Regular.ttf");
                    printf("Using Google Sans\n");
                }

                UI_PrefWidth(ui_pct(1, 1)) 
                {
                    ui_slider(&state->font_size, 6, 20, S8("Slider 1"));
                    state->font_size = (f32)ceil_f32_to_int(state->font_size);
                }

                ui_spacer(ui_em(1, 0));

                f32 text_padding_px = 5;

                UI_PrefWidth(ui_pct(1, 1)) UI_PrefHeight(ui_pct(0.5, 0))
                    UI_ChildLayoutAxis(UI_Axis2_Y)
                    UI_Parent(ui_box_makef(UI_BoxFlag_DrawBorder |
                                UI_BoxFlag_Clickable |
                                UI_BoxFlag_OverflowY |
                                UI_BoxFlag_ClipChildren, "text"))
                    UI_PrefHeight(ui_em(1.2, 1))
                    UI_PrefWidth(ui_tc(text_padding_px, 0))
                {
                    for (u32 line_idx = 0; 
                            line_idx < txt_get_n_lines(state->text);
                            line_idx++) {
                        String8 display_string = txt_get_line(state->frame_arena, state->text, line_idx);
                        String8 key_string = str8_cat(
                                frame_arena,
                                S8("line"),
                                str8_from_u32(frame_arena, line_idx));

                        UI_Box *line_box = ui_box_make(
                                // ((state->cursor_row == line_idx) ? UI_BoxFlag_DrawBorder : 0) |
                                UI_BoxFlag_DrawText,
                                key_string); 
                        ui_box_equip_string(line_box, display_string);

                        if (state->cursor_row == line_idx) {
                            f32 advance = 0; 
                            if (display_string.size) {
                                FC_GlyphRun *glyph_run = line_box->display_run;
                                u32 bytes_consumed = 0;
                                for (FC_GlyphPtrNode *glyph_ptr_n = glyph_run->first;
                                        glyph_ptr_n != 0 && bytes_consumed < state->cursor_col_bytes;
                                        glyph_ptr_n = glyph_ptr_n->next) {
                                    FC_Glyph *glyph = glyph_ptr_n->v;
                                    bytes_consumed += utf8_encode(glyph->codepoint, 0);
                                    advance += glyph->metrics.advance;
                                }
                            }

                            R_Bucket *line_bucket = r_get_new_bucket();
                            ui_box_equip_r_bucket(line_box, line_bucket);
                            line_box->r_bucket = line_bucket;
                            r_push_bucket(line_bucket);
                            {
                                f32 cursor_width = ui_top_font_size() / 10;
                                Rect2 cursor_rect = (Rect2){
                                    .V4 = V4(
                                            line_box->rect.min.x + advance + text_padding_px,
                                            line_box->rect.min.y,
                                            line_box->rect.min.x + advance + cursor_width + text_padding_px,
                                            line_box->rect.max.y),
                                };

                                r_push_rect2(.pos = cursor_rect);
                            }
                            r_pop_bucket();
                        }
                    }
                }
            }
        }
    }
    ui_end_build();

    ui_layout();
    ui_render();

    events->first = events->last = 0;
}
