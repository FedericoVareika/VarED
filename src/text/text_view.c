
internal TXT_ViewOp txt_op_from_view_action(Arena *arena, TXT_View *view, TXT_ViewAction action) {
    TXT_ViewOp op = {0};

    op.new_cursor = view->cursor;
    op.new_mark = view->mark;
    op.new_hor_anchor_em = view->horizontal_anchor_em;

    i32 insertion_col_delta = 0;

    if (action.codepoint) {
        u8 dst[4] = {0};
        u32 count = utf8_encode(action.codepoint, &dst[0]);
        String8 insert_text = {
            .str = &dst[0],
            .size = count,
        };
        op.insert_text = str8_copy(arena, insert_text);

        insertion_col_delta += count;
        op.update_cursor_col = true;
        action.flags |= TXT_ViewAction_Flag_Delete;
    }

    i32 col_delta = 0;
    i32 row_delta = action.row_delta;
    if ((i64)op.new_cursor.y + row_delta < 0) {
        row_delta = -(i64)op.new_cursor.y + 1;
    } else if ((u64)((i64)op.new_cursor.y + row_delta) > txt_get_n_lines(view->text)) {
        row_delta = txt_get_n_lines(view->text) - op.new_cursor.y;
    }

    // NOTE(fede): Consume characters
    // TODO(fede): Consume words
    if (action.hor_delta != 0) {
        op.update_cursor_col = true;

        Temp scratch = scratch_begin(&arena, 1);

        String8 line = txt_get_line(scratch.arena, view->text, op.new_cursor.y + row_delta);

        op.new_cursor.x = min(line.size, op.new_cursor.x); 

        if (!!(action.flags & TXT_ViewAction_Flag_ScanWords)) {
            col_delta = utf8_scan_words(line, op.new_cursor.x, action.hor_delta);
        } else {
            col_delta = utf8_scan_codepoints(line, op.new_cursor.x, action.hor_delta);
        }

        scratch_end(scratch);
    }

    if (!!(action.flags & TXT_ViewAction_Flag_SetHorizontalAnchor)) {
        op.new_hor_anchor_em = action.hor_anchor_em;
    }

    if (!!(action.flags & TXT_ViewAction_Flag_ZeroDeltaWithSelection) && 
            (view->cursor.x != view->mark.x || 
             view->cursor.y != view->mark.y)) {
        col_delta = 0;
        row_delta = 0;
    }

    op.new_cursor.x += col_delta;
    op.new_cursor.y += row_delta;

    op.new_cursor.y = max(op.new_cursor.y, 1);
    op.new_cursor.y = min(op.new_cursor.y, txt_get_n_lines(view->text));

    if (!!(action.flags & TXT_ViewAction_Flag_Delete)) {
        op.replace_range = rng2u(op.new_cursor, op.new_mark);
        op.new_cursor = op.new_mark = op.replace_range.min;
    }

    op.new_cursor.x += insertion_col_delta;

    if (!(action.flags & TXT_ViewAction_Flag_KeepMark)) {
        op.new_mark = op.new_cursor;
    } else {
        op.keep_mark = true;
    }

    return op;
}
