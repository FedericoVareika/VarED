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

#include "text/text_inc.c"

#include "vared.h"

void editor_init(EditorParams *params) {
    // STUDY(fede): change to *params->memory = arena_bootstrap_struct(EditorState, arena)
    // STUDY(fede): change commit/reserve sizes for this
    Arena *arena = arena_alloc();
    EditorState *state = push_struct(arena, EditorState);
    state->arena = arena;
    *params->memory = state;

    // STUDY(fede): change commit/reserve sizes for this
    state->frame_arena = arena_alloc();

    {
        state->input_view_n = push_struct(arena, TXT_ViewNode);
        TXT_View *view = &state->input_view_n->v;
        view->text_arena = arena_alloc();
        view->text = push_struct(view->text_arena, TXT_Text);
        view->single_line = true;
    }

    p_init();

    fc_init();
    fp_init();
    state->font = fp_open_font("data/fonts/GoogleSans-Regular.ttf");
    state->font_size = 14;

    ui_init();

    state->show_profiler = false;
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
                    if (!state->general_selected_view_n)
                        break;
                    TXT_View *view = &state->general_selected_view_n->v;

                    if (view->single_line)
                        break;

                    u64 at = txt_get_line_offset(view->text, view->cursor_row);
                    at += view->cursor_col;
                    txt_insert(view->text_arena, view->text, S8("\n"), at);
                    view->cursor_col = 0;
                    view->cursor_row++;
                } break;

                case WMKey_BACKSPACE: {
                    if (!state->general_selected_view_n)
                        break;
                    TXT_View *view = &state->general_selected_view_n->v;

                    u64 n = 0;
                    if (view->cursor_row == 0 && view->cursor_col == 0)
                        break;

                    Temp scratch = scratch_begin(0, 0);

                    String8 line = txt_get_line(scratch.arena, view->text, view->cursor_row);
                    do {
                        if (view->cursor_col > 0) {
                            view->cursor_col--;
                        } else {
                            if (view->cursor_row)
                                view->cursor_row--;

                            line = txt_get_line(scratch.arena, view->text, view->cursor_row);
                            view->cursor_col = line.size - 1; // before the \n
                        }
                        n++;
                    } while (!utf8_byte_is_header(line.str[view->cursor_col]));

                    u64 at = txt_get_line_offset(view->text, view->cursor_row);
                    at += view->cursor_col;
                    txt_delete(view->text_arena, view->text, at, n);

                    scratch_end(scratch);
                } break;

                case WMKey_LEFT: {
                    if (!state->general_selected_view_n)
                        break;
                    TXT_View *view = &state->general_selected_view_n->v;

                    if (view->cursor_row == 0 && view->cursor_col == 0)
                        break;

                    Temp scratch = scratch_begin(0, 0);
                    String8 line = txt_get_line(scratch.arena, view->text, view->cursor_row);
                    do {
                        if (view->cursor_col > 0) {
                            view->cursor_col--;
                        } else {
                            if (view->cursor_row)
                                view->cursor_row--;

                            line = txt_get_line(scratch.arena, view->text, view->cursor_row);
                            view->cursor_col = line.size - 1; // before the \n
                        }
                    } while (!utf8_byte_is_header(line.str[view->cursor_col]));

                    scratch_end(scratch);
                } break;
                case WMKey_RIGHT: {
                    if (!state->general_selected_view_n)
                        break;
                    TXT_View *view = &state->general_selected_view_n->v;

                    u32 new_cursor_col = view->cursor_col; 
                    u32 new_cursor_row = view->cursor_row; 

                    Temp scratch = scratch_begin(0, 0);
                    String8 line = txt_get_line(scratch.arena, view->text, view->cursor_row);
                    
                    if (new_cursor_col + 1 == line.size && line.str[new_cursor_col] == '\n')
                        new_cursor_col++;

                    do {
                        if (new_cursor_col < line.size) {
                            new_cursor_col++;
                        } else if (new_cursor_col == line.size) {
                            if (txt_get_n_lines(view->text) <= new_cursor_row + 1) {
                                new_cursor_col = view->cursor_col;
                                new_cursor_row = view->cursor_row;
                                break;
                            }

                            new_cursor_row++;
                            line = txt_get_line(scratch.arena, view->text, new_cursor_row);
                            new_cursor_col = 0;
                            break;
                        }
                    } while (!utf8_byte_is_header(line.str[new_cursor_col]));

                    view->cursor_col = new_cursor_col;
                    view->cursor_row = new_cursor_row;
                    scratch_end(scratch);
                } break;
                case WMKey_UP: {
                    if (!state->general_selected_view_n)
                        break;
                    TXT_View *view = &state->general_selected_view_n->v;

                    if (view->single_line)
                        break;

                    if (view->cursor_row > 0) {
                        view->cursor_row--;

                        // TODO(fede): Do optically aligned, instead of byte aligned.
                        Temp scratch = scratch_begin(0, 0);
                        String8 line = txt_get_line(scratch.arena, view->text, view->cursor_row);
                        view->cursor_col = min(line.size - 1, view->cursor_col);

                        while (!utf8_byte_is_header(line.str[view->cursor_col])) {
                            view->cursor_col--;
                        }
                        scratch_end(scratch);
                    }
                } break;
                case WMKey_DOWN: {
                    if (!state->general_selected_view_n)
                        break;
                    TXT_View *view = &state->general_selected_view_n->v;

                    if (view->single_line)
                        break;

                    if (view->cursor_row + 1 < txt_get_n_lines(view->text)) {
                        view->cursor_row++;

                        // TODO(fede): Do optically aligned, instead of byte aligned.
                        Temp scratch = scratch_begin(0, 0);
                        String8 line = txt_get_line(scratch.arena, view->text, view->cursor_row);
                        view->cursor_col = min(line.size - 1, view->cursor_col);

                        while (!utf8_byte_is_header(line.str[view->cursor_col])) {
                            view->cursor_col--;
                        }
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
                    if (!state->general_selected_view_n)
                        break;
                    TXT_View *view = &state->general_selected_view_n->v;

                    if (event.character) {
                        u8 insert_chars[4] = {0};
                        u32 codepoint_byte_size = utf8_encode(event.character, (u8 *)insert_chars);

                        u64 at = txt_get_line_offset(view->text, view->cursor_row);
                        at += view->cursor_col;
                        txt_insert(view->text_arena, view->text, str8((u8 *)&insert_chars, codepoint_byte_size), at);
                        view->cursor_col += codepoint_byte_size;
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
                    UI_Padding(ui_em(1, 0))
                    UI_ChildLayoutAxis(UI_Axis2_Y)
                    UI_PrefWidth(ui_pct(1, 0))
                    UI_Parent(ui_box_makef(0, "panel 1"))
                    UI_PrefHeight(ui_em(2, 1)) UI_PrefWidth(ui_tc(10, 0))
                {
                    ui_spacer(ui_em(1, 0));

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

                    ui_spacer(ui_em(0.5, 0));
                    
                    UI_PrefWidth(ui_pct(1, 0))
                        UI_Row 
                        UI_PrefWidth(ui_tc(10, 0))
                    {
                        if (ui_button(S8("Use Iosevka")).clicked) {
                            fp_close_font(state->font);
                            fc_flush();
                            state->font = fp_open_font("data/fonts/IosevkaTermNerdFontMono-Light.ttf");
                            printf("Using Iosevka\n");
                        }

                        ui_spacer(ui_em(1, 0));

                        if (ui_button(S8("Use Google Sans")).clicked) {
                            fp_close_font(state->font);
                            fc_flush();
                            state->font = fp_open_font("data/fonts/GoogleSans-Regular.ttf");
                            printf("Using Google Sans\n");
                        }
                    }

                    ui_spacer(ui_em(0.5, 0));

                    UI_PrefWidth(ui_pct(1, 1)) 
                    {
                        ui_slider(&state->font_size, 6, 20, S8("Font size"));
                        state->font_size = (f32)ceil_f32_to_int(state->font_size);
                    }

                    ui_spacer(ui_em(1, 0));

                    f32 text_padding_px = 5;
                    UI_PrefWidth(ui_pct(1, 1)) 
                        UI_Row
                    {
                        UI_PrefWidth(ui_pct(1, 0))
                        {
                            bool selected = state->general_selected_view_n == state->input_view_n;
                            UI_Comm text_view_comm = ui_text_view(
                                    frame_arena,
                                    &state->input_view_n->v,
                                    S8("input_text"),
                                    text_padding_px, selected);
                            if (text_view_comm.clicked) {
                                state->general_selected_view_n = state->input_view_n;
                            }
                        }

                        ui_spacer(ui_em(1, 0));

                        UI_PrefWidth(ui_tc(10, 1))
                        {
                            if (ui_button(S8("Open")).clicked) {
                                // TODO: open file command
                                TXT_View *input_view = &state->input_view_n->v;

                                TXT_ViewNode *view_n = push_struct(state->arena, TXT_ViewNode);
                                TXT_View *view = &view_n->v;
                                view->text_arena = arena_alloc();
                                view->text = push_struct(view->text_arena, TXT_Text);

                                DLL_PushBack(state->first_view, state->last_view, view_n);
                                
                                Temp scratch = scratch_begin(0, 0);
                                String8 path = txt_get_line(scratch.arena, input_view->text, input_view->cursor_row);
                                path = str8_strip(path);

                                view->label = str8_copy(state->arena, path);

                                char *path_cstr = cstr_from_str8(scratch.arena, path);
                                DebugReadFileResult file = debug_platform_read_entire_file(0, path_cstr);

                                if (!file.memory) {
                                    printf("Could not open file: %s\n", path_cstr);
                                    break;
                                }

                                arena_clear(view->text_arena);
                                view->text = push_struct(view->text_arena, TXT_Text);
                                view->cursor_row = 0;
                                view->cursor_col = 0;

                                txt_insert(view->text_arena, view->text, str8(file.memory, file.size), 0);

                                debug_platform_free_file_memory(0, file);
                                scratch_end(scratch);
                            }

                        }
                    }

                    bool draw_view = false;
                    UI_PrefWidth(ui_pct(1, 1)) 
                        UI_Row
                        UI_PrefWidth(ui_em(10, 0))
                        // UI_ChildLayoutAxis(UI_Axis2_Y)
                    {
                        u32 i = 0;
                        for (TXT_ViewNode *view_n = state->first_view;
                                view_n != 0; 
                                view_n = view_n->next, i++) {
                            TXT_View *view = &view_n->v;

                            bool main_selected = view_n == state->main_selected_view_n;
                            bool general_selected = view_n == state->general_selected_view_n;

                            if (general_selected)
                                ui_push_border_color(RGBA(1, 0, 0, 1));
                            else 
                                ui_push_border_color(RGBA(0, 0, 1, 1));

                            draw_view = draw_view || main_selected;

                            String8 text_box_label = S8("##text");
                            text_box_label = str8_cat(frame_arena, text_box_label, str8_from_u32(frame_arena, i));

                            UI_Box *view_tab_box = ui_box_make(
                                    UI_BoxFlag_DrawText |
                                    UI_BoxFlag_DrawBorder |
                                    UI_BoxFlag_DrawHotEffects |
                                    UI_BoxFlag_DrawActiveEffects |
                                    UI_BoxFlag_Clickable, 
                                    text_box_label);
                            ui_box_equip_string(view_tab_box, view->label);

                            if (ui_comm_from_box(view_tab_box).clicked) {
                                state->general_selected_view_n = view_n;
                                state->main_selected_view_n = view_n;
                                draw_view = true;
                            }

                            ui_pop_border_color();
                        }
                    }

                    if (draw_view) {
                        UI_PrefWidth(ui_pct(1, 0)) UI_PrefHeight(ui_pct(0.75, 0))
                        {
                            TXT_ViewNode *view_n = state->main_selected_view_n;
                            TXT_View *view = &view_n->v;

                            if (ui_text_view(frame_arena, view, S8("##text_box"), text_padding_px, view_n == state->general_selected_view_n).clicked) {
                                state->general_selected_view_n = view_n;
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

