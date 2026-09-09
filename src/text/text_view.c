
internal TXT_ViewOp txt_op_from_view_action(Arena *arena, TXT_View *view, TXT_ViewAction action) {
    TXT_ViewOp op = {0};

    op.new_cursor = view->cursor;
    op.new_mark = view->mark;

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
    }

    i32 col_delta = 0;
    i32 row_delta = 0;

    // NOTE(fede): Consume characters
    // TODO(fede): Consume words
    if (action.hor_char_delta != 0) {
        op.update_cursor_col = true;

        Temp scratch = scratch_begin(&arena, 1);

        String8 line = txt_get_line(scratch.arena, view->text, op.new_cursor.y);
        i32 sign = action.hor_char_delta / abs(action.hor_char_delta);
        assert(sign == 1 || sign == -1);

        bool end = false;
        while (!end && action.hor_char_delta != 0) {
            do {
                i64 new_cursor_col = (i64)op.new_cursor.x + col_delta + sign;
                if (new_cursor_col < 0) {
                    end = true; 
                    break;
                } else if (new_cursor_col > (u32)line.size) {
                    end = true; 
                    break;
                }                

                col_delta += sign;
            } while (!utf8_byte_is_header(line.str[op.new_cursor.x + col_delta]));

            action.hor_char_delta -= sign;
        }

        scratch_end(scratch);
    }

    if (!(action.flags & TXT_ViewAction_Flag_KeepBehindInsertion)) {
        col_delta += insertion_col_delta;
    }
    row_delta += action.row_delta;

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

    if (!(action.flags & TXT_ViewAction_Flag_KeepMark)) {
        op.new_mark = op.new_cursor;
    }

    return op;
}
