#include "base/base_inc.h"
#include "base/base_inc.c"

#include "thread_context/thread_context.h"
#include "thread_context/thread_context.c"

#define PROFILER 1
#include "profiler/profiler.h"
#include "profiler/profiler.c"

#include "commands/commands.h"
#include "commands/commands.c"

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
        view->cursor.y = 1;
        view->mark.y = 1;
    }

    p_init();

    fc_init();
    fp_init();
    state->font = fp_open_font("data/fonts/GoogleSans-Regular.ttf");
    state->font_size = 14;
    state->line_height = 1.2;

    ui_init();
    cmd_init();

    state->show_profiler = false;
}

void text_view(EditorState *state, TXT_ViewNode *view_n, String8 label) {
    TXT_View *view = &view_n->v;
    f32 text_padding_em = 0.4;

    // NOTE(fede): Apply per-frame actions
    bool reset_anchor = false;
    bool set_cursor_from_anchor = view->anchor_changed;
    {
        // TODO(fede): Make this be able to consume multiple actions (queue)
        TXT_ViewOp op = txt_op_from_view_action(state->frame_arena, view, view->action);
        view->action = (TXT_ViewAction){0};

        if (op.replace_range.min.y != 0) {
            u64 min_at = txt_get_line_offset(view->text, op.replace_range.min.y);
            min_at += op.replace_range.min.x;
            u64 max_at = txt_get_line_offset(view->text, op.replace_range.max.y);
            max_at += op.replace_range.max.x;
            if (max_at - min_at) {
                txt_delete(view->text_arena, view->text, min_at, max_at - min_at);
            }
        }

        if (op.insert_text.size) {
            u64 cursor_at = txt_get_line_offset(view->text, view->cursor.y);
            cursor_at += view->cursor.x;
            txt_insert(view->text_arena, view->text, op.insert_text, cursor_at);
        }

        if (op.update_cursor_col) {
            reset_anchor = true;
            view->cursor.x = op.new_cursor.x;
        }

        if (!view->single_line) {
            if (view->cursor.y != op.new_cursor.y) {
                set_cursor_from_anchor = true;
            }
            view->cursor.y = op.new_cursor.y;
        }

        view->mark = op.new_mark;
    }

    // NOTE(fede): If reset anchor is set, we should prioritize it, instead of 
    // resetting the cursor. 
    // This is because we might change the row of the cursor, but also change
    // the cursor column correctly.
    set_cursor_from_anchor &= !reset_anchor;

    // Align cursor to anchor or set anchor to cursor.
    if (reset_anchor || set_cursor_from_anchor) {
        Temp scratch = scratch_begin(0, 0);

        f32 one_em = ui_get_em(1, state->font_size);
        FC_GlyphRun *glyph_run = fc_get_string_glyph_run(
                state->font,
                txt_get_line(scratch.arena, view->text, view->cursor.y),
                state->font_size);

        u32 bytes_consumed = 0;
        f32 advance = 0;

        for (FC_GlyphPtrNode *glyph_ptr_n = glyph_run->first;
                glyph_ptr_n != 0;
                glyph_ptr_n = glyph_ptr_n->next) {
            FC_Glyph *glyph = glyph_ptr_n->v;

            if (reset_anchor &&
                    (bytes_consumed >= view->cursor.x || 
                    glyph->codepoint == (u32)'\n')) {
                view->horizontal_anchor_em = advance / one_em;
                break;
            } else if (set_cursor_from_anchor && 
                    (view->horizontal_anchor_em * one_em - advance < glyph->metrics.advance / 2 || 
                    glyph->codepoint == (u32)'\n')) {
                view->cursor.x = bytes_consumed;
                break;
            }

            bytes_consumed += utf8_encode(glyph->codepoint, 0);
            advance += glyph->metrics.advance;
        }

        view->anchor_changed = false;

        scratch_end(scratch);
    }

    UI_Comm view_comm = ui_text_view(state->frame_arena, view, label, text_padding_em, view_n == state->focused_view, state->line_height);

    if (view_comm.clicked) {
        Temp scratch = scratch_begin(0, 0);
        {
            CMD *cmd = cmd_push_name(S8("focus_view"));
            cmd->view_n = view_n;
        }

        f32 text_padding_px = ui_get_em(text_padding_em, view_comm.box->font_size);

        f32 mouse_line = (view_comm.rel_mouse_pos.y - text_padding_px) /
            ui_get_em(state->line_height, state->font_size);
        u32 new_cursor_row = (u32)mouse_line + view->line_offset + 1;
        new_cursor_row = min(new_cursor_row, txt_get_n_lines(view->text));

        f32 mouse_advance = view_comm.rel_mouse_pos.x - text_padding_px;

        {
            view->action.row_delta = (i32)new_cursor_row - (i32)view->cursor.y;
            view->horizontal_anchor_em = mouse_advance / ui_get_em(1, state->font_size);
            view->anchor_changed = true;
        }

        scratch_end(scratch);
    }
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
            // TODO(fede): Move these to commands.
            case WMEventKind_Press: {
            // case WMEventKind_Release: {
                switch (event.key) {

                case WMKey_RETURN: {
                    if (!state->focused_view)
                        break;
                    TXT_View *view = &state->focused_view->v;

                    if (view->single_line)
                        break;

                    view->action.codepoint = (u32)'\n';
                    view->action.row_delta++;
                    view->action.hor_char_delta = -I32_MAX;
                    view->action.flags |= 
                        TXT_ViewAction_Flag_KeepBehindInsertion;
                } break;

                case WMKey_BACKSPACE: {
                    if (!state->focused_view)
                        break;
                    TXT_View *view = &state->focused_view->v;
                    view->action.flags |= 
                        TXT_ViewAction_Flag_Delete |
                        TXT_ViewAction_Flag_ZeroDeltaWithSelection;
                    view->action.hor_char_delta--;
                } break;

                case WMKey_LEFT: {
                    if (!state->focused_view)
                        break;
                    TXT_View *view = &state->focused_view->v;
                    view->action.hor_char_delta--;
                } break;
                case WMKey_RIGHT: {
                    if (!state->focused_view)
                        break;
                    TXT_View *view = &state->focused_view->v;
                    view->action.hor_char_delta++;
                } break;
                case WMKey_UP: {
                    if (!state->focused_view)
                        break;
                    TXT_View *view = &state->focused_view->v;
                    view->action.row_delta--;
                } break;
                case WMKey_DOWN: {
                    if (!state->focused_view)
                        break;
                    TXT_View *view = &state->focused_view->v;
                    view->action.row_delta++;
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
                    if (!state->focused_view)
                        break;
                    TXT_View *view = &state->focused_view->v;

                    if (event.character) {
                        view->action.codepoint = event.character;
                        // view->action.hor_char_delta += 1;
                    }
                } break; 
                }
            } break;

            case WMEventKind_MouseMove: {
                
            } break;
            }
        }
    }

    CMD_List *commands = cmd_get_pending(); 
    for (CMD_Node *cmd_n = commands->first; 
            cmd_n != 0; 
            cmd_n = cmd_n->next) {
        CMD *cmd = &cmd_n->v;
        CMD_Kind kind = cmd_kind_from_name(cmd->name);

        switch (kind) {
        case CMD_Kind_OpenFile: {
            TXT_ViewNode *view_n = push_struct(state->arena, TXT_ViewNode);
            TXT_View *view = &view_n->v;
            view->text_arena = arena_alloc();
            view->text = push_struct(view->text_arena, TXT_Text);

            DLL_PushBack(state->first_view, state->last_view, view_n);
            
            Temp scratch = scratch_begin(0, 0);
            view->label = str8_copy(state->arena, cmd->filepath);

            char *path_cstr = cstr_from_str8(scratch.arena, cmd->filepath);
            DebugReadFileResult file = debug_platform_read_entire_file(0, path_cstr);

            if (!file.memory) {
                printf("Could not open file: %s\n", path_cstr);
                break;
            }

            arena_clear(view->text_arena);
            view->text = push_struct(view->text_arena, TXT_Text);
            view->file_view = true;
            view->cursor.y = 1;
            view->mark.y = 1;

            txt_insert(view->text_arena, view->text, str8(file.memory, file.size), 0);

            debug_platform_free_file_memory(0, file);
            scratch_end(scratch);
        } break;

        case CMD_Kind_FocusView: {
            TXT_ViewNode *view_n = cmd->view_n;
            state->focused_view = view_n;
            if (view_n->v.file_view) {
                state->selected_file_view = view_n;
            }
        } break;
        }
    }

    cmd_tick();

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

                    UI_PrefWidth(ui_pct(1, 1)) 
                        UI_Row
                    {
                        UI_PrefWidth(ui_pct(1, 0))
                        {
                            text_view(state, state->input_view_n, S8("input_text"));
                        }

                        ui_spacer(ui_em(1, 0));

                        UI_PrefWidth(ui_tc(10, 1))
                        {
                            if (ui_button(S8("Open")).clicked) {
                                TXT_View *input_view = &state->input_view_n->v;
                                String8 path = txt_get_line(cmd_frame_arena(), input_view->text, input_view->cursor.y);
                                path = str8_strip(path);

                                // TODO(fede): Command kind fast-paths.
                                CMD *cmd = cmd_push_name(S8("open"));
                                cmd->filepath = path;
                            }
                        }
                    }

                    f32 text_padding_px = 5;
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

                            bool focused = view_n == state->focused_view;
                            bool selected = view_n == state->selected_file_view;

                            if (selected)
                                ui_push_border_color(RGBA(1, 0, 0, 1));
                            else 
                                ui_push_border_color(RGBA(0, 0, 1, 1));

                            draw_view = draw_view || selected;

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
                                CMD *cmd = cmd_push_name(S8("focus_view"));
                                cmd->view_n = view_n;
                            }

                            ui_pop_border_color();
                        }
                    }

                    if (draw_view) {
                        UI_PrefWidth(ui_pct(1, 0)) UI_PrefHeight(ui_pct(0.75, 0))
                        {
                            text_view(state, state->selected_file_view, S8("##text_box"));
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
