/*
 * GENERAL PROJECT TODOs: 
 *
 *  - Embed files that are currently shipped in data:
 *      - Default font.
 *
 *  - Async I/O for file reading, writing. 
 *      - Maybe even file streaming, to not have the entire editing file in memory??
 *
 *  - Subpixel font rendering and alignment. 
 *
 *  - Lexer (at least C) for code highlighting and basic analysis. 
 *
 *  - More established UI: 
 *      - Tabs, panes, tree-style viewer for these. 
 *      - Control panel with settings and stuff. 
 *
 *  - Better movement: 
 *      - Vim or kakoune style movement
 *      - Multiple cursors? 
 *
 *  - Directory viewer for opening files visually, and creating files.
 *
 *  - App config, that is persistent (the following are needed regardless of persistent): 
 *      - Colours
 *      - Keybinds 
 *
 *  - Better profiler:
 *      - Floating pane
 *      - Historic graph (STUDY on how other people do this)
 *      - Memory profiler (arena analysis)
 *
 * */

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

#include "draw/draw.h"
#include "draw/draw.c"

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

// STUDY(fede): Editing lines in sqlite3.c (middle/bottom) produces spikes in 
//      this function. Check out further.
void text_view(EditorState *state, TXT_ViewNode *view_n, String8 label) {
    TimeFunction;

    TXT_View *view = &view_n->v;
    f32 text_padding_em = 0.4;

    // NOTE(fede): Apply per-frame actions
    bool reset_anchor = false;
    bool set_cursor_from_anchor = false;
    bool keep_mark = false;
    for (TXT_ViewActionNode *action_n = view->first_action;
            action_n != 0;
            action_n = action_n->next) {
        TXT_ViewAction *action = &action_n->v;
        TXT_ViewOp op = txt_op_from_view_action(state->frame_arena, view, *action);

        if (op.keep_mark)
            keep_mark = true;

        if (op.insert_text.size) {
            // TODO(fede): Give the option to insert before or after the selection area. 
            v2u end = view->cursor; 
            if (end.y < view->mark.y || end.y == view->mark.y && end.x < view->mark.x)
                end = view->mark;

            u64 end_at = txt_get_line_offset(view->text, end.y);
            end_at += end.x;
            txt_insert(view->text_arena, view->text, op.insert_text, end_at);
        }

        if (op.replace_range.min.y != 0) {
            u64 min_at = txt_get_line_offset(view->text, op.replace_range.min.y);
            min_at += op.replace_range.min.x;
            u64 max_at = txt_get_line_offset(view->text, op.replace_range.max.y);
            max_at += op.replace_range.max.x;
            if (max_at - min_at) {
                txt_delete(view->text_arena, view->text, min_at, max_at - min_at);
            }
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

        if (view->horizontal_anchor_em != op.new_hor_anchor_em) {
            view->horizontal_anchor_em = op.new_hor_anchor_em;
            set_cursor_from_anchor = true;
        }

        view->mark = op.new_mark;
        view->line_offset = op.new_line_offset;
    }
    view->first_action = 0;

    // NOTE(fede): If reset anchor is set, we should prioritize it, instead of 
    //      resetting the cursor. 
    //      This is because we might change the row of the cursor, but also
    //      change the cursor column correctly.
    set_cursor_from_anchor &= !reset_anchor;

    // NOTE(fede): After processing actions, visually align cursor to anchor or
    //      set anchor to cursor.
    if (reset_anchor || set_cursor_from_anchor) {
        Temp scratch = scratch_begin(0, 0);

        f32 one_em = ui_get_em(1, state->font_size);
        FC_GlyphRun *glyph_run = fc_get_string_glyph_run(
                state->font,
                txt_get_line(scratch.arena, view->text, view->cursor.y),
                state->font_size);

        u32 bytes_consumed = 0;
        f32 advance = 0;

        FC_GlyphPtrNode *glyph_ptr_n = glyph_run->first;
        while (true) {
            if (glyph_ptr_n == 0) 
                break;

            FC_Glyph *glyph = glyph_ptr_n->v;

            if (set_cursor_from_anchor) {
                f32 horizontal_anchor_px = view->horizontal_anchor_em * one_em;
                f32 glyph_center = advance + glyph->metrics.advance / 2;
                bool reached_anchor = glyph_center > horizontal_anchor_px;

                if (reached_anchor) {
                    break;
                }
            } else if (reset_anchor) {
                bool reached_target_bytes = bytes_consumed >= view->cursor.x;

                if (reached_target_bytes) {
                    break;
                }
            }

            if (glyph->codepoint == (u32)'\n')
                break;

            bytes_consumed += utf8_encode(glyph->codepoint, 0);
            advance += glyph->metrics.advance;

            glyph_ptr_n = glyph_ptr_n->next; 
        }

        if (set_cursor_from_anchor) {
            view->cursor.x = bytes_consumed;
            if (!keep_mark) {
                view->mark.x = bytes_consumed;
            }
        } else if (reset_anchor) {
            view->horizontal_anchor_em = advance / one_em;
        }

        scratch_end(scratch);
    }

    bool selected = view_n == state->focused_view;

    // NOTE(fede): Build main text view UI.
    UI_Parent(ui_box_make(0, label))
        UI_BackgroundColor(ui_lighten_color(ui_top_background_color(), 0.1))
        UI_Row
    {
        f32 estimated_lines_in_box;

        UI_ChildLayoutAxis(UI_Axis2_Y)
        {
            UI_Box *text_box = ui_box_makef(
                    UI_BoxFlag_DrawBorder |
                    UI_BoxFlag_Clickable |
                    UI_BoxFlag_Draggable |
                    UI_BoxFlag_Scrollable |
                    UI_BoxFlag_DrawBackground |
                    UI_BoxFlag_OverflowY |
                    UI_BoxFlag_ClipChildren, "##text_box");

            f32 line_height_px = (u32)ui_get_em(state->line_height, text_box->font_size);

            estimated_lines_in_box = text_box->rect.max.y - text_box->rect.min.y;
            estimated_lines_in_box /= line_height_px;
            estimated_lines_in_box += 1;

            f32 text_padding_px = ui_get_em(text_padding_em, text_box->font_size);

            UI_Comm view_comm = ui_comm_from_box(text_box);

            // NOTE(fede): Draw text lines in text box.
            {
                Temp scratch = scratch_begin(0, 0);

                UI_Box *text_box = view_comm.box;

                R_Bucket *text_bucket = r_get_new_bucket();
                ui_box_equip_r_bucket(text_box, text_bucket);
                r_push_bucket(text_bucket);

                Rng2u64 line_range = {
                    .min = floor_f32_to_int(view->line_offset) + 1,
                    .max = floor_f32_to_int(view->line_offset) + ceil_f32_to_int(estimated_lines_in_box),
                };

                v2 at = {
                    text_box->rect.min.x + text_padding_px,
                    text_box->rect.min.y,
                };

                if (view->single_line) {
                    // Center text on text box
                    f32 center = (text_box->rect.max.y + text_box->rect.min.y) / 2;
                    at.y = center - (line_height_px / 2);
                } else {
                    // Do fractional vertical scrolling
                    if (view->sub_line_offset) {
                        at.y -= view->sub_line_offset * line_height_px;
                    }
                }

                Rng2u selection_range = rng2u(view->mark, view->cursor);

                {
                    TimeBlock(S8("Draw lines"));
                    u32 line_idx = 1;
                    for (u32 line_num = line_range.min; 
                            line_num <= line_range.max && line_idx <= txt_get_n_lines(view->text);
                            line_num++, line_idx++) {

                        String8 line_string = txt_get_line(scratch.arena, view->text, line_num);
                        FC_GlyphRun *glyph_run = fc_get_string_glyph_run(state->font, line_string, state->font_size);
                        FP_FontMetrics metrics = fp_get_font_metrics(state->font, state->font_size);

                        dr_glyph_run(metrics, glyph_run, at, text_box->rect, text_box->text_color);

                        bool line_is_selected = selected && 
                            line_num >= selection_range.min.y &&
                            line_num <= selection_range.max.y;
                        // NOTE(fede): Draw cursor and selection.
                        if (line_is_selected) {
                            f32 selection_start = 0;
                            f32 selection_end = 0;

                            f32 cursor_width = max(ui_top_font_size() / 10, 1);
                            f32 cursor_advance = 0; 
                            f32 mark_advance = 0; 

                            if (line_string.size) {
                                selection_end = glyph_run->advance;

                                if (view->cursor.y == line_num || view->mark.y == line_num) {
                                    bool cursor_set = false;
                                    bool mark_set = false;

                                    f32 advance = 0;

                                    u32 bytes_consumed = 0;

                                    FC_GlyphPtrNode *glyph_ptr_n = glyph_run->first;
                                    while (true) {
                                        if (!cursor_set) {
                                            cursor_advance = advance;
                                            cursor_set = bytes_consumed == view->cursor.x;
                                        }
                                        if (!mark_set) {
                                            mark_advance = advance;
                                            mark_set = bytes_consumed == view->mark.x;
                                        }

                                        if (cursor_set && mark_set)
                                            break;

                                        if (glyph_ptr_n == 0) 
                                            break;

                                        FC_Glyph *glyph = glyph_ptr_n->v;
                                        bytes_consumed += utf8_encode(glyph->codepoint, 0);
                                        advance += glyph->metrics.advance;

                                        glyph_ptr_n = glyph_ptr_n->next;
                                    }

                                }
                            }

                            if (view->cursor.y == line_num) {
                                if (v2u_equal(selection_range.min, view->cursor)) {
                                    selection_start = cursor_advance;
                                } else {
                                    selection_end = cursor_advance;
                                }

                                Rect2 cursor_rect = (Rect2){
                                    .V4 = V4(
                                            at.x + cursor_advance,
                                            at.y,
                                            at.x + cursor_advance + cursor_width,
                                            at.y + line_height_px),
                                };

                                r_push_rect2(.pos = cursor_rect, .clip = text_box->rect);
                            }

                            if (view->mark.y == line_num) {
                                if (v2u_equal(selection_range.min, view->mark)) {
                                    selection_start = mark_advance;
                                } else {
                                    selection_end = mark_advance;
                                }
                            }

                            if (!v2u_equal(selection_range.min, selection_range.max) &&
                                    selection_start != selection_end) {
                                assert(selection_start < selection_end);
                                Rect2 selection_rect = (Rect2){
                                    .V4 = V4(
                                            at.x + selection_start,
                                            at.y,
                                            at.x + selection_end,
                                            at.y + line_height_px),
                                };
                                r_push_rect2(.pos = selection_rect, .clip = text_box->rect, R_Color4(RGBA(1, 0, 0, 0.5)));
                            }
                        }

                        at.y += line_height_px;
                    }
                }

                r_pop_bucket();
                scratch_end(scratch);

                view->first_line = view->line_offset;
                view->last_line = view->line_offset + (u32)estimated_lines_in_box;
            }

            if (view_comm.pressed) {
                {
                    CMD *cmd = cmd_push_name(S8("focus_view"));
                    cmd->view_n = view_n;
                }
            }

            if (view_comm.dragging) {
                Temp scratch = scratch_begin(0, 0);

                f32 mouse_y = view_comm.rel_mouse_pos.y - text_padding_px;
                if (mouse_y < 0)
                    mouse_y = 0;

                f32 mouse_line = mouse_y /
                    ui_get_em(state->line_height, state->font_size);
                u32 new_cursor_row = (u32)mouse_line + view->line_offset + 1;
                new_cursor_row = min(new_cursor_row, txt_get_n_lines(view->text));

                f32 mouse_advance = view_comm.rel_mouse_pos.x - text_padding_px;

                {
                    CMD *cmd = cmd_push_name(S8("text_action"));
                    cmd->view_n = view_n;
                    cmd->view_action.row_delta = (i32)new_cursor_row - (i32)view->cursor.y;
                    cmd->view_action.hor_anchor_em = mouse_advance / ui_get_em(1, state->font_size); 
                    cmd->view_action.flags |= 
                        TXT_ViewAction_Flag_SetHorizontalAnchor |
                        (!view_comm.pressed ? TXT_ViewAction_Flag_KeepMark : 0);
                }

                scratch_end(scratch);
            }

            if (view_comm.scroll_delta.y) {
                f32 delta_line_offset_f = view_comm.scroll_delta.y;
                // TODO(fede): Parametize sensitivity
                f32 new_sub_line_offset = view->sub_line_offset - 2 * delta_line_offset_f;
                i32 delta_line_offset = floor_f32_to_int(new_sub_line_offset);
                new_sub_line_offset -= delta_line_offset;
                assert(new_sub_line_offset >= 0 && new_sub_line_offset < 1);

                if (delta_line_offset < 0) {
                    if (view->line_offset < (u64)(-delta_line_offset)) {
                        view->line_offset = 0;
                        new_sub_line_offset = 0;
                    } else {
                        view->line_offset = view->line_offset - (u64)(-delta_line_offset);
                    }
                } else { 
                    if (view->line_offset + delta_line_offset > txt_get_n_lines(view->text) - 3) {
                        view->line_offset = txt_get_n_lines(view->text) - 3;
                        new_sub_line_offset = 1;
                    } else {
                        view->line_offset += delta_line_offset;
                    }
                }

                view->sub_line_offset = new_sub_line_offset;
            }
        }

        if (!view->single_line) {
            f64 min_scroller_size = 0.05;
            f64 max = (f64)txt_get_n_lines(view->text) - 2;
            f64 scroller_size = max(min_scroller_size, (f64)estimated_lines_in_box / max);

            f64 line_pct = (f64)view->line_offset + (f32)view->sub_line_offset;
            line_pct /= max;

            UI_PrefWidth(ui_em(1, 1))
                UI_ChildLayoutAxis(UI_Axis2_Y)
                    ui_slider_anon(&line_pct, 0, 1, scroller_size, S8("##line_slider"));
            
            f64 line_offset_f = line_pct * max; 
            view->line_offset = (u64)(line_offset_f);
            view->sub_line_offset = line_offset_f - (f32)(view->line_offset);
        }
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
                    TXT_ViewNode *view_n = state->focused_view;
                    TXT_View *view = &view_n->v;

                    if (view->single_line)
                        break;


                    {
                        CMD *cmd = cmd_push_name(S8("text_action"));
                        cmd->view_n = view_n;
                        cmd->view_action.codepoint = (u32)'\n';
                    }

                    {
                        CMD *cmd = cmd_push_name(S8("text_action"));
                        cmd->view_n = view_n;
                        cmd->view_action.flags |= TXT_ViewAction_AutoScrollLines;
                        cmd->view_action.row_delta++;
                        cmd->view_action.hor_delta = -I32_MAX;
                    }

                } break;

                case WMKey_BACKSPACE: {
                    if (!state->focused_view)
                        break;
                    TXT_ViewNode *view_n = state->focused_view;
                    TXT_View *view = &view_n->v;

                    CMD *cmd = cmd_push_name(S8("text_action"));
                    cmd->view_n = view_n;
                    cmd->view_action.hor_delta--;
                    cmd->view_action.flags |= 
                        TXT_ViewAction_Flag_Delete |
                        TXT_ViewAction_Flag_ZeroDeltaWithSelection;
                    if (!!(event.modifiers & WMModifier_shift) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_KeepMark;
                    if (!!(event.modifiers & WMModifier_ctrl) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_ScanWords;
                } break;

                case WMKey_DELETE: {
                    if (!state->focused_view)
                        break;
                    TXT_ViewNode *view_n = state->focused_view;
                    TXT_View *view = &view_n->v;

                    CMD *cmd = cmd_push_name(S8("text_action"));
                    cmd->view_n = view_n;
                    cmd->view_action.hor_delta++;
                    cmd->view_action.flags |= 
                        TXT_ViewAction_Flag_Delete |
                        TXT_ViewAction_Flag_ZeroDeltaWithSelection;
                    if (!!(event.modifiers & WMModifier_shift) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_KeepMark;
                    if (!!(event.modifiers & WMModifier_ctrl) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_ScanWords;
                } break;

                case WMKey_LEFT: {
                    if (!state->focused_view)
                        break;
                    TXT_ViewNode *view_n = state->focused_view;
                    TXT_View *view = &view_n->v;

                    CMD *cmd = cmd_push_name(S8("text_action"));
                    cmd->view_n = view_n;
                    cmd->view_action.hor_delta--;
                    if (!!(event.modifiers & WMModifier_shift) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_KeepMark;
                    if (!!(event.modifiers & WMModifier_ctrl) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_ScanWords;
                } break;
                case WMKey_RIGHT: {
                    if (!state->focused_view)
                        break;
                    TXT_ViewNode *view_n = state->focused_view;
                    TXT_View *view = &view_n->v;

                    CMD *cmd = cmd_push_name(S8("text_action"));
                    cmd->view_n = view_n;
                    cmd->view_action.hor_delta++;
                    if (!!(event.modifiers & WMModifier_shift) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_KeepMark;
                    if (!!(event.modifiers & WMModifier_ctrl) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_ScanWords;
                } break;
                case WMKey_UP: {
                    if (!state->focused_view)
                        break;
                    TXT_ViewNode *view_n = state->focused_view;
                    TXT_View *view = &view_n->v;

                    CMD *cmd = cmd_push_name(S8("text_action"));
                    cmd->view_n = view_n;
                    cmd->view_action.row_delta--;
                    cmd->view_action.flags |= TXT_ViewAction_AutoScrollLines;
                    if (!!(event.modifiers & WMModifier_shift) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_KeepMark;
                    if (!!(event.modifiers & WMModifier_ctrl) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_ScanWords;
                } break;
                case WMKey_DOWN: {
                    if (!state->focused_view)
                        break;
                    TXT_ViewNode *view_n = state->focused_view;
                    TXT_View *view = &view_n->v;

                    CMD *cmd = cmd_push_name(S8("text_action"));
                    cmd->view_n = view_n;
                    cmd->view_action.row_delta++;
                    cmd->view_action.flags |= TXT_ViewAction_AutoScrollLines;
                    if (!!(event.modifiers & WMModifier_shift) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_KeepMark;
                    if (!!(event.modifiers & WMModifier_ctrl) )
                        cmd->view_action.flags |= TXT_ViewAction_Flag_ScanWords;
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
                    TXT_ViewNode *view_n = state->focused_view;
                    TXT_View *view = &view_n->v;

                    if (event.character) {
                        CMD *cmd = cmd_push_name(S8("text_action"));
                        cmd->view_n = view_n;
                        cmd->view_action.codepoint = event.character;
                        cmd->view_action.flags |= TXT_ViewAction_AutoScrollLines;
                    }
                } break; 
                }
            } break;

            case WMEventKind_MouseScroll:
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

        case CMD_Kind_TextAction: {
            TXT_ViewNode *view_n = cmd->view_n;
            TXT_ViewAction action = cmd->view_action;

            TXT_ViewActionNode *action_n = push_struct(state->arena, TXT_ViewActionNode);
            action_n->v = action;
            
            SLL_PushBack(view_n->v.first_action, view_n->v.last_action, action_n);
        } break;

        default: {} break;
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
            UI_CornerRadius(ui_get_em(0.2, state->font_size))
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

                    {
                        P_FrameState *p_prev = p_previous_state();
                        u64 total_clocks = p_prev->end_time - p_prev->start_time;
                        f64 total_time_frame_pct = ((f64)total_clocks / performance_frequency()) * 144;

                        if (false && total_time_frame_pct > 1) {
                            printf("Total clocks: %lu (Cpu freq: %lu)\n", total_clocks, performance_frequency());
                            for (u32 i = 1; i <= p_prev->last_anchor_idx; i++) {
                                P_Anchor *anchor = &p_prev->anchors[i];
                                f64 pct = (f64)anchor->exclusive_elapsed_time * 100 / (f64)total_clocks;

                                printf("  %.*s[%lu]: %lu (%.2f%%", 
                                        (int)anchor->label.size,
                                        anchor->label.str,
                                        anchor->hit_count,
                                        anchor->exclusive_elapsed_time, pct);

                                if (anchor->inclusive_elapsed_time != anchor->exclusive_elapsed_time) {
                                    f64 percentage_inclusive =
                                        (f64)(anchor->inclusive_elapsed_time * 100) / (f64)total_clocks;
                                    printf(", %.2f%% w/children", percentage_inclusive);
                                }

                                if (anchor->processed_byte_count) {
                                    f64 megabyte = 1024.0f * 1024.0f;

                                    f64 seconds = (f64)anchor->inclusive_elapsed_time / (f64)performance_frequency();
                                    f64 bytes_per_second = (f64)anchor->processed_byte_count / seconds;
                                    f64 megabytes = (f64)anchor->processed_byte_count / (f64)megabyte;
                                    f64 megabytes_per_second = bytes_per_second / megabyte;

                                    // printf("  %.3fmb at %.2fgb/s", megabytes, gigabytes_per_second);
                                    printf("  %.3fmb at %.2fmb/s", megabytes, megabytes_per_second);
                                }
                                printf(")\n");
                            }
                        }

                        if (state->show_profiler) {
                            UI_PrefWidth(ui_pct(1, 1))
                                UI_PrefHeight(ui_cs(1))
                            {
                                bool red = total_time_frame_pct > 1;

                                UI_NamedColumn(S8("__anchors__"))
                                    UI_PrefHeight(ui_em(1.2, 1))
                                for (u32 i = 1; i <= p_prev->last_anchor_idx; i++) {
                                    P_Anchor *anchor = &p_prev->anchors[i];
                                    f64 pct = (f64)anchor->exclusive_elapsed_time / (f64)total_clocks;

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
                                        {
                                            UI_Box *pct_box = ui_box_make(UI_BoxFlag_DrawText, S8(""));
                                            ui_box_equip_string(pct_box, str8_from_f32(frame_arena, pct * 100, 2));
                                        }

                                        UI_CornerRadius(0)
                                            UI_BackgroundColor(RGBA(red, !red, 0, 1))
                                            UI_PrefWidth(ui_pct(pct / 2, 1))
                                            ui_box_make(UI_BoxFlag_DrawBackground, S8(""));

                                        if (anchor->processed_byte_count) {
                                            f64 kilobytes = (f64)anchor->processed_byte_count / (f64)kilobytes(1);

                                            ui_spacer(ui_pct(1, 0));

                                            UI_PrefWidth(ui_em(5, 0))
                                            {
                                                UI_Box *bandwidth = ui_box_make(UI_BoxFlag_DrawText, S8(""));
                                                ui_box_equip_string(bandwidth, str8_from_f32(frame_arena, kilobytes, 2));
                                            }
                                        }
                                    }

                                    scratch_end(scratch);
                                }
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

                    ui_spacer(ui_em(1, 0));

                    f32 text_padding_px = 5;
                    bool draw_view = false;
                    UI_PrefWidth(ui_pct(1, 1)) 
                        UI_Row
                        UI_PrefWidth(ui_em(10, 0))
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

                            UI_TextPadding(10)
                            {
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
                            }


                            ui_pop_border_color();
                        }
                    }

                    if (draw_view) {
                        UI_PrefWidth(ui_pct(1, 0)) UI_PrefHeight(ui_pct(1, 0))
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
