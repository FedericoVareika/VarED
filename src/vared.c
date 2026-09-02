#include "base/base_inc.h"
#include "base/base_inc.c"

#include "thread_context/thread_context.h"
#include "thread_context/thread_context.c"

#define PROFILER 1
#include "profiler/profiler.h"
#include "profiler/profiler.c"

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

void editor_init(EditorParams *params) {
    // STUDY(fede): change to *params->memory = arena_bootstrap_struct(EditorState, arena)
    // STUDY(fede): change commit/reserve sizes for this
    Arena *arena = arena_alloc();
    EditorState *state = push_struct(arena, EditorState);
    state->arena = arena;
    *params->memory = state;

    // STUDY(fede): change commit/reserve sizes for this
    state->frame_arena = arena_alloc(.commit_size=megabytes(1));

    state->text_arena = arena_alloc();
    state->text = push_struct(state->text_arena, TXT_Text);

    p_init();

    fc_init();
    fp_init();
    state->font = fp_open_font("data/fonts/GoogleSans-Regular.ttf");
    state->font_size = 14;

    ui_init();

    state->show_profiler = true;
    state->cull_lines = true;
}

void editor_update_and_render(EditorParams *params) {
    TimeFunction;

    EditorState *state = (EditorState *)*params->memory;
    Arena *frame_arena = state->frame_arena;

    arena_clear(frame_arena);

    fc_tick();

    WMEventList *events = params->events;
    
    {
        TimeBlock(S8("Event consume"));
        for (WMEventNode *event_n = events->first; event_n; event_n = event_n->next) {
            WMEvent event = event_n->v;
            switch (event.kind) {
            case WMEventKind_Press: {
            // case WMEventKind_Release: {
                switch (event.key) {

                case WMKey_RETURN: {
                    u64 at = txt_get_line_offset(state->text, state->cursor_row);
                    at += state->cursor_col_bytes;
                    txt_insert(state->text_arena, state->text, S8("\n"), at);
                    state->cursor_col_bytes = 0;
                    state->cursor_row++;
                } break;

                case WMKey_BACKSPACE: {
                    u64 n = 0;
                    if (state->cursor_row == 0 && state->cursor_col_bytes == 0)
                        break;

                    Temp scratch = scratch_begin(0, 0);

                    String8 line = txt_get_line(scratch.arena, state->text, state->cursor_row);
                    do {
                        if (state->cursor_col_bytes > 0) {
                            state->cursor_col_bytes--;
                        } else {
                            if (state->cursor_row)
                                state->cursor_row--;

                            line = txt_get_line(scratch.arena, state->text, state->cursor_row);
                            state->cursor_col_bytes = line.size - 1; // before the \n
                        }
                        n++;
                    } while (!utf8_byte_is_header(line.str[state->cursor_col_bytes]));

                    u64 at = txt_get_line_offset(state->text, state->cursor_row);
                    at += state->cursor_col_bytes;
                    txt_delete(state->text_arena, state->text, at, n);

                    scratch_end(scratch);
                } break;

                case WMKey_LEFT: {
                    if (state->cursor_row == 0 && state->cursor_col_bytes == 0)
                        break;

                    Temp scratch = scratch_begin(0, 0);
                    String8 line = txt_get_line(scratch.arena, state->text, state->cursor_row);
                    do {
                        if (state->cursor_col_bytes > 0) {
                            state->cursor_col_bytes--;
                        } else {
                            if (state->cursor_row)
                                state->cursor_row--;

                            line = txt_get_line(scratch.arena, state->text, state->cursor_row);
                            state->cursor_col_bytes = line.size - 1; // before the \n
                        }
                    } while (!utf8_byte_is_header(line.str[state->cursor_col_bytes]));

                    scratch_end(scratch);
                } break;
                case WMKey_RIGHT: {
                    u32 new_cursor_col_bytes = state->cursor_col_bytes; 
                    u32 new_cursor_row = state->cursor_row; 

                    Temp scratch = scratch_begin(0, 0);
                    String8 line = txt_get_line(scratch.arena, state->text, state->cursor_row);
                    
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
                            line = txt_get_line(scratch.arena, state->text, new_cursor_row);
                            new_cursor_col_bytes = 0;
                            break;
                        }
                    } while (!utf8_byte_is_header(line.str[new_cursor_col_bytes]));

                    state->cursor_col_bytes = new_cursor_col_bytes;
                    state->cursor_row = new_cursor_row;
                    scratch_end(scratch);
                } break;
                case WMKey_UP: {
                    if (state->cursor_row > 0) {
                        state->cursor_row--;

                        // TODO(fede): Do optically aligned, instead of byte aligned.
                        Temp scratch = scratch_begin(0, 0);
                        String8 line = txt_get_line(scratch.arena, state->text, state->cursor_row);
                        state->cursor_col_bytes = min(line.size - 1, state->cursor_col_bytes);

                        while (!utf8_byte_is_header(line.str[state->cursor_col_bytes])) {
                            state->cursor_col_bytes--;
                        }
                        scratch_end(scratch);
                    }
                } break;
                case WMKey_DOWN: {
                    if (state->cursor_row + 1 < txt_get_n_lines(state->text)) {
                        state->cursor_row++;

                        // TODO(fede): Do optically aligned, instead of byte aligned.
                        Temp scratch = scratch_begin(0, 0);
                        String8 line = txt_get_line(scratch.arena, state->text, state->cursor_row);
                        state->cursor_col_bytes = min(line.size - 1, state->cursor_col_bytes);

                        while (!utf8_byte_is_header(line.str[state->cursor_col_bytes])) {
                            state->cursor_col_bytes--;
                        }
                        scratch_end(scratch);
                    }
                } break;

                case WMKey_o: {
                    if ((event.modifiers & WMModifier_ctrl) && 
                        !(event.modifiers & WMModifier_shift) &&
                        !(event.modifiers & WMModifier_alt)) {
                        Temp scratch = scratch_begin(0, 0);
                        String8 path = txt_get_line(scratch.arena, state->text, state->cursor_row);
                        path = str8_strip(path);

                        char *path_cstr = cstr_from_str8(scratch.arena, path);
                        DebugReadFileResult file = debug_platform_read_entire_file(0, path_cstr);

                        if (!file.memory) {
                            printf("Could not open file: %s\n", path_cstr);
                            break;
                        }

                        u64 at = txt_get_line_offset(state->text, state->cursor_row);
                        at += state->cursor_col_bytes;
                        txt_insert(state->text_arena, state->text, str8(file.memory, file.size), at);

                        debug_platform_free_file_memory(0, file);
                        scratch_end(scratch);
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
    }

    v2 window_dim = {
        .x = r_state->window_width,
        .y = r_state->window_height,
    };
    R_Bucket *main_bucket = r_get_new_bucket();
    r_push_bucket(main_bucket);

    {
        TimeBlock(S8("UI Building"));

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
                    if (ui_button(S8("Profiler")).clicked) {
                        state->show_profiler = !state->show_profiler;
                    }

                    if (state->show_profiler) {
                        UI_PrefWidth(ui_pct(1, 1))
                            UI_PrefHeight(ui_cs(1))
                        {
                            P_FrameState *p_prev = p_previous_state();
                            u64 total_time = p_prev->end_time - p_prev->start_time;
                            f64 total_time_frame_pct = ((f64)total_time / performance_frequency()) * 144;

                            bool red = total_time_frame_pct > 1;

                            f64 added_pct = 0;

                            UI_NamedColumn(S8("__anchors__"))
                                UI_PrefHeight(ui_em(1.2, 1))
                            for (u32 i = 1; i <= p_prev->last_anchor_idx; i++) {
                                P_Anchor *anchor = &p_prev->anchors[i];
                                f64 pct = (f64)anchor->exclusive_elapsed_time / (f64)total_time;
                                added_pct += pct;

                                Temp scratch = scratch_begin(0, 0);

                                String8 parent_label = str8_cat(frame_arena, S8("__anchor__"), str8_from_u64(frame_arena, i));
                                UI_ChildLayoutAxis(UI_Axis2_X)
                                    UI_PrefWidth(ui_pct(1, 1))
                                    UI_Parent(ui_box_make(UI_BoxFlag_ClipChildren, parent_label))
                                {
                                    UI_PrefWidth(ui_em(10, 1))
                                        UI_Parent(ui_box_makef(UI_BoxFlag_ClipChildren, ""))
                                        ui_box_make(UI_BoxFlag_DrawText, anchor->label);

                                    UI_PrefWidth(ui_em(5, 1))
                                        ui_box_make(UI_BoxFlag_DrawText, str8_from_u32(frame_arena, (u32)(pct * 100)));

                                    UI_CornerRadius(0)
                                        UI_BackgroundColor(RGBA(red, !red, 0, 1))
                                        UI_PrefWidth(ui_pct(pct / 2, 1))
                                        ui_box_make(UI_BoxFlag_DrawBackground, S8(""));

                                    if (anchor->processed_byte_count) {
                                        f64 kilobytes = (f64)anchor->processed_byte_count / (f64)kilobytes(1);

                                        ui_spacer(ui_pct(1, 0));

                                        UI_PrefWidth(ui_em(5, 0))
                                            ui_box_make(UI_BoxFlag_DrawText, str8_from_u32(frame_arena, (u32)(kilobytes)));
                                    }
                                }

                                scratch_end(scratch);
                            }
                        }
                    }
                    
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
                        ui_slider(&state->font_size, 6, 20, S8("Font size"));
                        state->font_size = (f32)ceil_f32_to_int(state->font_size);
                    }

                    UI_PrefWidth(ui_cs(1))
                        ui_checkbox(&state->cull_lines, S8("Cull Lines"));

                    ui_spacer(ui_em(1, 0));

                    f32 text_padding_px = 5;

                    UI_PrefWidth(ui_pct(1, 1)) UI_PrefHeight(ui_pct(0.5, 0))
                        UI_ChildLayoutAxis(UI_Axis2_Y)
                    {
                        UI_Box *text_box = ui_box_makef(
                                UI_BoxFlag_DrawBorder |
                                UI_BoxFlag_Clickable |
                                UI_BoxFlag_OverflowY |
                                UI_BoxFlag_ClipChildren, "text");

                        f32 line_height = 1.2;

                        f32 estimated_lines_in_box = text_box->rect.max.y - text_box->rect.min.y;
                        estimated_lines_in_box /= ui_get_em(line_height, text_box->font_size);
                        estimated_lines_in_box += 2;

                        UI_Parent(text_box)
                            UI_PrefHeight(ui_em(line_height, 1))
                            UI_PrefWidth(ui_tc(text_padding_px, 0))
                        {
                            TimeBlock(S8("UI Build Text"));

                            ui_spacer(ui_em(1, 0));

                            for (u32 line_idx = 0; 
                                    line_idx < txt_get_n_lines(state->text);
                                    line_idx++) {

                                if (state->cull_lines && line_idx > estimated_lines_in_box) {
                                    break;
                                }

                                String8 display_string = txt_get_line(frame_arena, state->text, line_idx);
                                String8 key_string = str8_cat(
                                        frame_arena,
                                        S8("line"),
                                        str8_from_u32(frame_arena, line_idx));

                                UI_Box *line_box = ui_box_make(
                                        UI_BoxFlag_DrawText,
                                        key_string); 
                                ui_box_equip_string(line_box, display_string);

                                if (state->cursor_row == line_idx) {
                                    f32 advance = 0; 
                                    if (display_string.size) {
                                        FC_GlyphRun *glyph_run = ui_get_box_display_run(line_box);
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
        }
        ui_end_build();
    }

    {
        TimeBlock(S8("UI Layout"))
        ui_layout();
    }

    {
        TimeBlock(S8("UI render"))
        ui_render();
    }

    events->first = events->last = 0;
}

