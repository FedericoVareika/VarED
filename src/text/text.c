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
    // text->first_free_piece works as a SLL stack
    piece_n->prev = 0; 
    piece_n->next = text->first_free_piece;
    text->first_free_piece = piece_n;
} 

internal inline u64 txt_piece_line_start_count(TXT_Piece *piece) {
    assert(piece);
    return piece->end.line_idx - piece->start.line_idx;
}

internal TXT_LinePos txt_advance_line_pos(TXT_Buffer *buffer, TXT_LinePos pos, u64 n) {
    TXT_LinePos result = pos;

    u64 start_byte = buffer->line_starts[result.line_idx];
    start_byte += result.offset;
    u64 end_byte = start_byte + n;


    u64 min_line = result.line_idx;
    u64 max_line = buffer->line_count;

    // NOTE(fede): Binary search for large pieces.
    while (true) {
        if (min_line + 1 == max_line) {
            result.line_idx = min_line;
            break;
        }

        u64 result_line = (max_line + min_line) / 2;
        u64 line_start = buffer->line_starts[result_line];

        if (line_start > end_byte) {
            max_line = result_line;
        } else {
            min_line = result_line;
        }
    }

    u64 max_offset = buffer->count - buffer->line_starts[result.line_idx];
    result.offset = end_byte - buffer->line_starts[result.line_idx];
    result.offset = min(result.offset, max_offset);

    return result;
}

internal TXT_PieceNode *txt_split_piece_n(
        Arena *arena, TXT_Text *text,
        TXT_PieceNode *piece_n, u64 at, u64 gap) {
    assert(piece_n);

    TXT_Piece *piece = &piece_n->v;
    assert(at <= piece->size); 

    if (at == piece->size) {
        return piece_n;
    }

    TXT_Buffer *buffer = piece->buffer;

    u64 l_size = at;
    u64 r_size = piece->size - at; // - gap;
    r_size = max(r_size, gap) - gap;

    TXT_PieceNode *insertion_node = piece_n->prev; 
    if (l_size > 0) {
        TXT_PieceNode *left_n = txt_get_piece_n(arena, text);

        TXT_LinePos left_end = txt_advance_line_pos(buffer, piece->start, l_size - 1);
        u64 left_newlines = left_end.line_idx - piece->start.line_idx;

        if (txt_line_pos_is_end_of_line(buffer, left_end))
            left_newlines++;

        left_n->v = (TXT_Piece){
            .buffer = piece->buffer,
            .start = piece->start,
            .end = left_end,
            .size = l_size,
            .newline_count = left_newlines,
        };

        DLL_Insert(text->first, text->last, insertion_node, left_n);
        insertion_node = left_n;
    }

    if (r_size > 0) {
        TXT_PieceNode *right_n = txt_get_piece_n(arena, text);

        TXT_LinePos right_start = txt_advance_line_pos(buffer, piece->start, at + gap);
        u64 right_newlines = piece->end.line_idx - right_start.line_idx;

        if (txt_line_pos_is_end_of_line(buffer, piece->end))
            right_newlines++;

        right_n->v = (TXT_Piece){
            .buffer = piece->buffer,
            .start = right_start,
            .end = piece->end,
            .size = r_size,
            .newline_count = right_newlines,
        };

        DLL_Insert(text->first, text->last, insertion_node, right_n);
    }

    txt_remove_piece_n(text, piece_n);

    return insertion_node;
}

internal bool txt_line_pos_is_end_of_line(TXT_Buffer *buffer, TXT_LinePos pos) {
    if (pos.line_idx + 1 >= buffer->line_count) {
        assert(pos.line_idx + 1 == buffer->line_count);

        return false;
    }

    u64 expanded_pos = buffer->line_starts[pos.line_idx] + pos.offset;
    u64 end_of_line = buffer->line_starts[pos.line_idx + 1] - 1;
    return expanded_pos == end_of_line;
}

internal inline bool txt_line_pos_is_continuation(TXT_Buffer *buffer, TXT_LinePos a, TXT_LinePos b) {
    bool result = a.line_idx == b.line_idx &&
        a.offset + 1 == b.offset;

    result |= txt_line_pos_is_end_of_line(buffer, a) &&
        a.line_idx + 1 == b.line_idx &&
        b.offset == 0;

    return result;
}

// STUDY(fede): The entire file is stored in memory right now,
//      is there a common alternative, is there any *good* alternative?
//      File streaming?
internal void txt_insert(Arena *arena, TXT_Text *text, String8 str, u64 at) {
    TimeFunctionBandwidth(str.size);
    assert(str.size);

    TXT_BufferNode *buffer_n = text->buffer_queue; 

    u64 newline_count = 0;
    for (u64 i = 0; i < str.size; i++) {
        if (str.str[i] == '\n') {
            newline_count++;
        }
    }

    if (!buffer_n || str.size > buffer_n->v.size - buffer_n->v.count 
        || newline_count > TXT_WRITE_BUFFER_MAX_LINES - buffer_n->v.line_count) {
        buffer_n = push_struct(arena, TXT_BufferNode);
        buffer_n->next = text->buffer_queue;
        text->buffer_queue = buffer_n;

        TXT_Buffer *buffer = &buffer_n->v;

        buffer->size = max(TXT_WRITE_BUFFER_SIZE, str.size);
        buffer->buf = push_array(arena, u8, buffer->size);

        // Consider this insert as a large paste or file open.
        // TODO(fede): Investigate and support CRLF, LF, and CR modes
        if (buffer->size > TXT_WRITE_BUFFER_SIZE || newline_count > TXT_WRITE_BUFFER_MAX_LINES) {
            buffer->line_starts = push_array(arena, u64, newline_count + 1);
        } else {
            buffer->line_starts = push_array(arena, u64, TXT_WRITE_BUFFER_MAX_LINES);
        }

        // NOTE(fede): The result contents are not updated
        buffer->line_count = 1; 
    }

    TXT_Buffer *buffer = &buffer_n->v;

    u64 piece_newline_count = 0; 

    // NOTE(fede): The piece should include start and end bytes.
    //      Like so:
    //          [start, end] <- inclusive
    //      Not: 
    //          [start, end) <- exclusive
    TXT_LinePos start = {0};
    start.line_idx = buffer->line_count - 1;
    start.offset = buffer->count - buffer->line_starts[start.line_idx]; 
    TXT_LinePos end = {0};

    mem_copy(buffer->buf + buffer->count, str.str, str.size);

    for (u64 i = 0; i < str.size; i++) {
        if (str.str[i] == '\n') {
            buffer->line_starts[buffer->line_count++] = buffer->count + i + 1;
            piece_newline_count++;
        }
    }

    buffer->count += str.size;

    // NOTE(fede): Set the line idx to the last inserted byte line idx, if 
    // the last byte is a '\n', we don't want it to have the end.line_idx be
    // on the next line. This reassures us of inclusive ranges, not exclusive.
    end.line_idx = buffer->line_count - 1;
    if (buffer->buf[buffer->count - 1] == '\n')
        end.line_idx--;
    
    // NOTE(fede): The -1 is because the range is inclusive: [start, end]
    end.offset = buffer->count - buffer->line_starts[end.line_idx] - 1; 

#ifdef VARED_INTERNAL
    if (str.size == 1) {
        assert(start.line_idx == end.line_idx);
        assert(start.offset == end.offset);
    }
#endif // VARED_INTERNAL

    TXT_Piece piece = (TXT_Piece){
        .buffer = buffer,
        .start = start,
        .end = end,
        .size = str.size,
        .newline_count = piece_newline_count,
    };

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
    u64 offset = at;
    TXT_PieceNode *piece_n_at = text->first; 
    for (; piece_n_at != 0; piece_n_at = piece_n_at->next) {
        if (piece_n_at->v.size >= offset)
            break;
        offset -= piece_n_at->v.size;
    }

    // Split the piece.
    // TODO(fede): if this is at a limit of two pieces, then instead of inserting 
    //      a node in the middle, it would delete the right one, insert the new
    //      one, and add the right one again.
    if (piece_n_at) {
        TXT_Piece *piece_at = &piece_n_at->v;
        if (piece_at->buffer == buffer && offset == piece_at->size) {
            if (txt_line_pos_is_continuation(buffer, piece_at->end, start)) {
                *piece_at = (TXT_Piece) {
                    .buffer = buffer,
                    .start = piece_at->start,
                    .end = end,
                    .size = piece_at->size + piece.size,
                    .newline_count = piece_at->newline_count + piece.newline_count,
                };

                return;
            } else {
                offset -= piece_n_at->v.size;
                piece_n_at = piece_n_at->next;
            }
        }
    }

    TXT_PieceNode *piece_n = txt_get_piece_n(arena, text);
    piece_n->v = piece;

    if (piece_n_at) {
        TXT_PieceNode *gap = txt_split_piece_n(arena, text, piece_n_at, offset, 0);
        DLL_Insert(text->first, text->last, gap, piece_n);
    } else {
        DLL_PushBack(text->first, text->last, piece_n);
    }
}

internal u64 txt_delete_from_node(
        Arena *arena, TXT_Text *text,
        TXT_PieceNode *node, u64 at, u64 n) {
    assert(node);
    assert(n);

    TXT_Piece *piece = &node->v;
    assert(at < piece->size);

    u64 n_deleted = n;
    if (piece->size <= at + n) {
        n_deleted = piece->size - at;
    }

    TXT_PieceNode *gap = txt_split_piece_n(arena, text, node, at, n);

    return n_deleted;
}

internal void txt_delete(Arena *arena, TXT_Text *text, u64 at, u64 n) {
    TimeFunction;
    u64 offset = at;
    TXT_PieceNode *delete_n = text->first;
    for (; delete_n != 0; delete_n = delete_n->next) {
        if (delete_n->v.size > offset)
            break;
        offset -= delete_n->v.size;
    }

    u64 n_deleted = 0;
    TXT_PieceNode *delete_next_n;
    for (; delete_n != 0; delete_n = delete_next_n) {
        delete_next_n = delete_n->next;
        n_deleted += txt_delete_from_node(arena, text, delete_n, offset, n - n_deleted);

        if (n_deleted >= n) {
            assert(n_deleted == n);
            break;
        }

        offset = 0;
    }
}

// TODO(fede): Cache this?
internal u64 txt_get_n_lines(TXT_Text *text) {
    u64 result = 1;
    for (TXT_PieceNode *piece_n = text->first; 
            piece_n != 0; 
            piece_n = piece_n->next) {
        TXT_Piece *piece = &piece_n->v;
        result += piece->newline_count;
    }

    return result;
}

internal u64 txt_get_line_offset(TXT_Text *text, u64 row) {
    assert(row > 0); 
    row--;

    u64 result = 0;
    u64 line_idx = 0;
    TXT_PieceNode *piece_n = text->first;
    if (!piece_n)
        return 0;

    for (; piece_n != 0; 
            piece_n = piece_n->next) {
        TXT_Piece *piece = &piece_n->v;
        u64 n_line_starts = txt_piece_line_start_count(piece);

        if (line_idx + n_line_starts >= row) {
            break;
        }

        line_idx += piece->newline_count;
        result += piece->size;
    }

    if (!piece_n) {
        assert(line_idx == row);
        return result;
    }

    /*
     * NOTE(fede): I have a brute approx, now i have to increase the result by 
     *      the internal piece offset to the line i want to get.
     *
     *              pn.start          row     pn.end
     *         \n      |      \n      \n         |
     *          |______|_______|_______|_________|
     *                 |               |         |
     *                 |_______________|         
     *                 |  amnt to add  |
     *              cur.res         fin.res
     *                         
     */

    
    TXT_Piece *piece = &piece_n->v;
    TXT_Buffer *buffer = piece->buffer;
    TXT_LinePos buf_line = piece->start;
    if (row != line_idx) {
        u64 lines_to_go = row - line_idx;
        u64 end_buf_line_idx = buf_line.line_idx + lines_to_go;

        u64 size_to_add = 
            buffer->line_starts[end_buf_line_idx] - 
            buffer->line_starts[buf_line.line_idx] - buf_line.offset;

        result += size_to_add;
    }

    return result;
}

// [start, end]
internal String8 txt_get_buffer_substr(
        TXT_Buffer *buffer,
        TXT_LinePos start, TXT_LinePos end) {
    assert(start.line_idx < end.line_idx ||
            start.line_idx == end.line_idx && 
            start.offset <= end.offset);

    /*
     *  NOTE(fede): To make sure the +-1 is correct, check with this 
     *      complicated diagram that is kind of readable.
     *
     *                     l  pn.start    l       l     pn.end
     *                     |     |        |       |        |
     *               ..._____________________________________________...
     *                    ||     |       |       ||        |
     *                   \n|     |      \n      \n|        |
     *  pn.start.offset => [----][-------------------------] <== size
     *                     |  6              27   |        |
     *                     |                      |        |
     *      end - start => [----------------------][-------] <= pn.end.offset
     *                                24               9
     *                                  
     *      pn.start.line_idx: n
     *      pn.start.offset: 6
     *      pn.end.line_idx: n + 2
     *      pn.end.offset: 9
     *
     *  NOTE(fede): Since we are using unsigned integers, change the 
     *          -(line_start-1) to (-line_start + 1) in code
     *
     *      pn.end.line_start - (pn.start.line_start - 1) = 24
     *
     *      24 + pn.end.offset = 24 + 9 = 33
     *      33 - pn.start.offset = 33 - 6 = 27
     *
     *      size = 27
     *
     * */

    u64 size = 
        buffer->line_starts[end.line_idx] -
        buffer->line_starts[start.line_idx] + 1;
    size += end.offset;
    size -= start.offset;

    u64 buf_offset = buffer->line_starts[start.line_idx] + start.offset;
    return str8(buffer->buf + buf_offset, size);
}

// NOTE(fede): Ends with \n if its not the end of text
// TODO(fede): Use txt_get_range? 
internal String8 txt_get_line(Arena *arena, TXT_Text *text, u64 row) {
    assert(row > 0);
    row--;
        
    u64 line_idx = 0;
    TXT_PieceNode *piece_n = text->first;

    // NOTE(fede): Get the LinePos of the start of the `row` 
    TXT_LinePos line_start_pos = {0}; 
    {
        TimeBlock(S8("GetLinePos"));
        for (; piece_n != 0; 
                piece_n = piece_n->next) {
            TXT_Piece *piece = &piece_n->v;
            u64 n_line_starts = txt_piece_line_start_count(piece);

            if (line_idx + n_line_starts >= row) {
                break;
            }

            line_idx += piece->newline_count;
        }

        if (!piece_n) 
            return str8(0, 0);

        TXT_Piece *piece = &piece_n->v;
        TXT_Buffer *buffer = piece->buffer;

        line_start_pos = piece->start;

        u64 lines_remaining = row - line_idx;
        if (lines_remaining > 0) {
            line_start_pos.line_idx = min(
                    buffer->line_count,
                    line_start_pos.line_idx + lines_remaining);
            line_start_pos.offset = 0;
        } 

        assert(line_start_pos.line_idx < piece->end.line_idx ||
                line_start_pos.line_idx == piece->end.line_idx &&
                line_start_pos.offset <= piece->end.offset);
    }


    // NOTE(fede): Do the string cats
    Temp scratch = scratch_begin(&arena, 1);
    String8 result = S("");
    {
        TimeBlock(S8("BuildLine"));
        TXT_Piece *piece = &piece_n->v;
        TXT_Buffer *buffer = piece->buffer;

        while (true) {
            TXT_LinePos line_end_pos = piece->end;
            if (line_start_pos.line_idx < line_end_pos.line_idx) {
                assert(buffer->line_starts[line_start_pos.line_idx + 1] >
                        buffer->line_starts[line_start_pos.line_idx]);
                line_end_pos.offset =
                    buffer->line_starts[line_start_pos.line_idx + 1] -
                    buffer->line_starts[line_start_pos.line_idx] - 1;
                line_end_pos.line_idx = line_start_pos.line_idx;
            }

            String8 buf_substr = 
                txt_get_buffer_substr(buffer, line_start_pos, line_end_pos);

            result = str8_cat(scratch.arena, result, buf_substr);

            if (txt_line_pos_is_end_of_line(buffer, line_end_pos)) {
                break;
            }

            piece_n = piece_n->next;
            if (!piece_n) {
                break;
            }

            piece = &piece_n->v;
            line_start_pos = piece->start;
            buffer = piece->buffer;
        }
    }
    
    result = str8_copy(arena, result);
    scratch_end(scratch);

    return result;
}

internal String8 txt_get_range(Arena *arena, TXT_Text *text, Rng2u rng) {
    // NOTE(fede): This is what we are getting, the start and end positions + their piece nodes.
    // Start is index 0, End is index 1.
    TXT_PieceNode *pieces_n[2] = {0};
    TXT_LinePos line_positions[2] = {0};
    {
        u64 start_row = rng.min.y;
        assert(start_row > 0);
        start_row--;

        u64 end_row = rng.max.y;
        assert(end_row > 0);
        end_row--;

        u64 rows[2] = { start_row, end_row };
        u64 cols[2] = { rng.min.x, rng.max.x };

        u64 line_idx = 0;
        TXT_PieceNode *iter_piece_n = text->first;

        for (u32 i = 0; i < 2; i++) {
            // NOTE(fede): Get the piece at the line rows[i]
            for (; iter_piece_n != 0; 
                    iter_piece_n = iter_piece_n->next) {
                TXT_Piece *piece = &iter_piece_n->v;
                u64 n_line_starts = txt_piece_line_start_count(piece);

                if (line_idx + n_line_starts >= rows[i]) {
                    break;
                }

                line_idx += piece->newline_count;
            }

            if (!iter_piece_n) 
                return str8(0, 0);

            TXT_LinePos line_pos;
            TXT_Piece *piece;
            TXT_Buffer *buffer;
            // NOTE(fede): Now we scan the pieces for the piece that contains cols[i] at this line.
            for (; iter_piece_n != 0; 
                    iter_piece_n = iter_piece_n->next) {
                piece = &iter_piece_n->v;
                buffer = piece->buffer;

                line_pos = piece->start;
                u64 lines_remaining = rows[i] - line_idx;
                // If there are lines remaining, move to that line in the piece
                if (lines_remaining > 0) {
                    line_pos.line_idx = min(
                            buffer->line_count,
                            line_pos.line_idx + lines_remaining);
                    line_pos.offset = 0;
                    lines_remaining = 0;
                } 
                assert(line_pos.line_idx < piece->end.line_idx ||
                        line_pos.line_idx == piece->end.line_idx &&
                        line_pos.offset <= piece->end.offset);

                // If this is not the last line in the piece, we assume that 
                // it is big enough for the cols[i] position
                if (piece->end.line_idx >= line_pos.line_idx) {
                    line_pos.offset += cols[i];
                    break;
                }

                // It must be the last line, so check if it is big enough, 
                // this is because if the piece is not large enough, then the 
                // next piece might be, and so on.
                assert(piece->end.line_idx == line_pos.line_idx);
                u32 space_left_in_line = piece->size - line_pos.offset;
                if (space_left_in_line >= cols[i]) {
                    // The piece is large enough, so choose it and update the offset.
                    line_pos.offset += cols[i];
                    break;
                } 

                // The piece is not large enough, so reduce the column offset 
                // and look in the next one.
                cols[i] -= space_left_in_line;
            }

            if (!iter_piece_n) 
                return str8(0, 0);
            
            // End iteration, assign.
            line_positions[i] = line_pos;
            pieces_n[i] = iter_piece_n;
        }
    }

    // NOTE(fede): Do the string cats between start and end positions, these 
    // might include different pieces, so copy all of the text.
    Temp scratch = scratch_begin(&arena, 1);
    String8 result = S("");
    {
        TimeBlock(S8("BuildTextRange"));
        TXT_PieceNode *start_piece_n = pieces_n[0];
        TXT_PieceNode *end_piece_n = pieces_n[1];

        TXT_PieceNode *piece_n = start_piece_n;
        TXT_LinePos substr_start = line_positions[0];

        while (true) {
            TXT_Piece *piece = &piece_n->v;
            TXT_Buffer *buffer = piece->buffer;

            TXT_LinePos substr_end = piece->end;

            if (piece_n == end_piece_n) {
                substr_end = line_positions[1];
            }

            String8 buf_substr = txt_get_buffer_substr(buffer, substr_start, substr_end);

            result = str8_cat(scratch.arena, result, buf_substr);

            if (piece_n == end_piece_n) {
                break;
            }

            piece_n = piece_n->next;
            assert(piece_n);
            substr_start = piece_n->v.start;
        }
    }

    result = str8_copy(arena, result);
    result.str[result.size - 1] = 0;
    scratch_end(scratch);

    printf("%ld\n", result.size);

    return result;
}


