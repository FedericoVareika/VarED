internal TXT_PieceNode *txt_get_piece_n(Arena *arena, TXT_Text *text) {
    TXT_PieceNode *result;

    if (text->first_free_piece) {
        result = text->first_free_piece; 
        text->first_free_piece = text->first_free_piece->next;
    } else {
        result = push_struct(arena, TXT_PieceNode);
    }

    return result;
} 

internal void txt_remove_piece_n(TXT_Text *text ,TXT_PieceNode *piece_n) {
    DLL_Remove(text->first, text->last, piece_n);
    piece_n->next = text->first_free_piece;
    text->first_free_piece = piece_n;
} 

internal TXT_LinePos txt_advance_line_pos(TXT_Buffer *buffer, TXT_LinePos pos, u64 n) {
    TXT_LinePos result = pos;
    
    u64 advanced = 0;
    while (true) {
        u64 from = buffer->line_starts[result.line_idx] + result.offset;
        u64 to = buffer->line_starts[result.line_idx];
        if (from - to > n - advanced) {
            break;    
        }

        advanced += from - to;
        result.line_idx++;
        result.offset = 0;
    }
    assert(n > advanced);
    result.offset = n - advanced;
    
    return result;
}

internal TXT_Buffer *txt_get_buffer_for_str(Arena *arena, TXT_Text *text, String8 str) {
    TXT_BufferNode *buffer_n = text->buffer_queue; 

    // Init first buffer. 
    if (!buffer_n) {
        assert(!text->first_free_piece);
        assert(!text->first);
        assert(!text->last);

        buffer_n = push_struct(arena, TXT_BufferNode);
        buffer_n->next = text->buffer_queue;
        text->buffer_queue = buffer_n;

        buffer_n->v.buf = push_array(arena, u8, str.size);
        buffer_n->v.size = str.size;

        assert(str.size > buffer_n->v.size - buffer_n->v.count);
    }

    TXT_Buffer *result = &buffer_n->v;

    if (str.size > result->size - result->count) {
        buffer_n = push_struct(arena, TXT_BufferNode);
        buffer_n->next = text->buffer_queue;
        text->buffer_queue = buffer_n;

        result = &buffer_n->v;

        result->size = max(TXT_WRITE_BUFFER_SIZE, str.size);
        result->buf = push_array(arena, u8, result->size);

        // Consider this insert as a large paste or file open.
        if (result->size > TXT_WRITE_BUFFER_SIZE) {
            u64 line_count = 1;

            // TODO(fede): Investigate and support CRLF, LF, and CR modes
            for (u64 i = 0; i < str.size; i++) {
                if (str.str[i] == '\n') {
                    line_count++;
                }
            }

            result->line_starts = push_array(arena, u64, line_count);
            // NOTE(fede): The result contents are not updated
            result->line_count = 1; 
        } else {
            result->line_starts = push_array(arena, u64, TXT_WRITE_BUFFER_MAX_LINES);
        }
    }

    return result;
}

// STUDY(fede): The entire file is stored in memory right now,
//      is there a common alternative, is there any *good* alternative?
//      File streaming?
internal void txt_insert(Arena *arena, TXT_Text *text, String8 str, u64 at) {
    TXT_Buffer *buffer = txt_get_buffer_for_str(arena, text, str);

    TXT_LinePos start = {0};
    start.line_idx = buffer->line_count;
    start.offset = buffer->count - buffer->line_starts[buffer->line_count]; 

    for (u64 i = 0; i < str.size; i++) {
        assert(buffer->line_count < TXT_WRITE_BUFFER_MAX_LINES);

        u64 buf_idx = i + buffer->count;
        if (str.str[i] == '\n') {
            buffer->line_starts[buffer->line_count] = buf_idx;
            buffer->line_count++;

            // STUDY perf
            buffer->buf[buf_idx] = str.str[i];
        }
    }

    TXT_LinePos end = {0};
    start.line_idx = buffer->line_count;
    start.offset = buffer->count - buffer->line_starts[buffer->line_count]; 

    TXT_PieceNode *piece_n = txt_get_piece_n(arena, text);

    piece_n->v = (TXT_Piece){
        .buffer = buffer,
        .start = start,
        .end = end,
        .size = str.size,
    };

    u64 offset = at;
    TXT_PieceNode *piece_n_at = text->first; 
    /*
     *  Offset becomes the offset from the start of piece_n_at, to the place 
     *  where we want to insert the new piece: 
     *
     *      p_n = piece_n_at
     *      \n = line starts
     *      off = offset
     * 
     *              pn.start                  pn.end
     *         \n      |      \n                 |
     *          |______|_______|_________________|
     *                 |               |         |
     *                 |_______________|
     *                 |      off      |
     */
    // TODO(fede): if this is at a limit of two pieces, then instead of inserting 
    //      a node in the middle, it would delete the right one, insert the new
    //      one, and add the right one again.
    for (; piece_n_at != 0, piece_n_at->v.size <= offset;
            piece_n_at = piece_n_at->next, offset -= piece_n_at->v.size) {
    }

    // Split the piece.
    if (piece_n_at) {
        u64 size_l = offset; 
        u64 size_r = piece_n_at->v.size - size_l;

        /*
         *  insert_pos becomes the line pos relative to piece_n_at where the 
         *  offset is (where we want to insert)
         *
         *      pn = piece_n_at
         *      \n = line starts
         *      i.off = insert_pos.offset
         *      l1 = insert_pos.line
         * 
         *              pn.start                  pn.end
         *         l0      |      l1                 |
         *          |______|_______|_________________|
         *                 |       |       |         |
         *                         |_______|
         *                         | i.off |
         *
         */
        TXT_LinePos insert_pos = txt_advance_line_pos(buffer, piece_n_at->v.end, offset);

        TXT_PieceNode *insertion_node = piece_n_at;
        if (size_l) {
            TXT_PieceNode *left_n = txt_get_piece_n(arena, text);

            left_n->v = (TXT_Piece){
                .buffer = buffer,
                .start = piece_n_at->v.start,
                .end = insert_pos,
            };

            DLL_Insert(text->first, text->last, insertion_node, left_n);
            insertion_node = left_n;

            insert_pos.offset++;
        }

        DLL_Insert(text->first, text->last, insertion_node, piece_n);
        insertion_node = piece_n;

        if (size_r) {
            TXT_PieceNode *right_n = txt_get_piece_n(arena, text);

            right_n->v = (TXT_Piece){
                .buffer = buffer,
                .start = insert_pos,
                .end = piece_n_at->v.end,
            };

            DLL_Insert(text->first, text->last, insertion_node, right_n);
        } else {
            // TODO(fede): We can merge with this piece if the buffer is the same
        }

        txt_remove_piece_n(text, piece_n_at);
    } else {
        DLL_PushBack(text->first, text->last, piece_n);
    }
}

internal void txt_delete(Arena *arena, TXT_Text *text, u64 at, u64 n) {
    u64 offset = at;
    TXT_PieceNode *first_to_delete = text->first;
    for (; first_to_delete != 0, first_to_delete->v.size <= offset;
            first_to_delete = first_to_delete->next, offset -= first_to_delete->v.size) {
    } 
    TXT_LinePos start = txt_advance_line_pos(
            first_to_delete->v.buffer,
            first_to_delete->v.start,
            offset);
    u64 new_first_piece_size = offset;

    offset = n;
    u32 pieces_to_delete_fully = 0;
    TXT_PieceNode *last_to_delete = first_to_delete;
    for (; last_to_delete != 0, last_to_delete->v.size <= offset;
            last_to_delete = last_to_delete->next, offset -= last_to_delete->v.size) {
        pieces_to_delete_fully++;
    } 
    pieces_to_delete_fully--;

    TXT_LinePos end = txt_advance_line_pos(
            last_to_delete->v.buffer,
            last_to_delete->v.start,
            offset);
    u64 new_end_piece_size = last_to_delete->v.size - offset;

    {
        TXT_PieceNode *delete = first_to_delete->next;
        for (u32 i = 0; i < pieces_to_delete_fully; i++) {
            TXT_PieceNode *delete_next = delete->next;
            txt_remove_piece_n(text, delete);
            delete = delete_next;
        }

        assert(!pieces_to_delete_fully && first_to_delete == last_to_delete || 
                pieces_to_delete_fully && delete == last_to_delete);
    }

    {
        if (new_first_piece_size) {
            TXT_PieceNode *new_n = txt_get_piece_n(arena, text);
            new_n->v = first_to_delete->v;
            new_n->v.end = start;
            new_n->v.size = new_first_piece_size;
        }
        txt_remove_piece_n(text, first_to_delete);

        if (new_end_piece_size) {
            TXT_PieceNode *new_n = txt_get_piece_n(arena, text);
            new_n->v = last_to_delete->v;
            new_n->v.start = end;
            new_n->v.size = new_end_piece_size;
        }
        txt_remove_piece_n(text, last_to_delete);
    }

    u64 deleted_amount = 0;
    for (TXT_PieceNode *delete_n = first_to_delete;
            deleted_amount < n, delete_n != 0;
            delete_n = delete_n->next) {
        TXT_Piece *delete = &delete_n->v;
        TXT_Buffer *buffer = delete->buffer;

        {
            TXT_LinePos left_end = start;
            if (left_end.offset == 0) {
                assert(left_end.line_idx > 0);
                left_end.offset = buffer->line_starts[left_end.line_idx] -
                    buffer->line_starts[left_end.line_idx - 1];
                left_end.line_idx--;
            } else {
                left_end.offset--;
            }

            TXT_PieceNode *insertion_n = delete_n;
            if (left_end.line_idx > delete->start.line_idx || 
                    left_end.line_idx == delete->start.line_idx && 
                    left_end.offset > delete->start.offset) {

                TXT_PieceNode *left_n = txt_get_piece_n(arena, text);

                left_n->v = (TXT_Piece){
                    .buffer = buffer,
                    .start = delete->start,
                    .end = left_end,
                };

                DLL_Insert(text->first, text->last, insertion_n, left_n);
                insertion_n = left_n;
            }

            TXT_LinePos right_start = end;
            if (right_start.line_idx < delete->end.line_idx || 
                    right_start.line_idx == delete->end.line_idx && 
                    right_start.offset < delete->end.offset) {

                TXT_PieceNode *right_n = txt_get_piece_n(arena, text);

                right_n->v = (TXT_Piece){
                    .buffer = buffer,
                    .start = right_start,
                    .end = end,
                };

                DLL_Insert(text->first, text->last, insertion_n, right_n);
            }

            txt_remove_piece_n(text, delete_n);
        }
    }
}
