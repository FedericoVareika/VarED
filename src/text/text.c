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

    for (; result.line_idx + 1 < buffer->line_count;
            result.line_idx++) {
        if (buffer->line_starts[result.line_idx + 1] > end_byte) {
            break;
        }
    }

    // NOTE(fede): Prevent advancing past the buffer end.
    u64 max_offset = buffer->count - buffer->line_starts[result.line_idx];
    result.offset = end_byte - buffer->line_starts[result.line_idx];
    result.offset = min(result.offset, max_offset);

    return result;
}

internal TXT_Buffer *txt_get_buffer_for_str(Arena *arena, TXT_Text *text, String8 str) {
    TXT_BufferNode *buffer_n = text->buffer_queue; 

    if (!buffer_n || str.size > buffer_n->v.size - buffer_n->v.count) {
        buffer_n = push_struct(arena, TXT_BufferNode);
        buffer_n->next = text->buffer_queue;
        text->buffer_queue = buffer_n;

        TXT_Buffer *buffer = &buffer_n->v;

        buffer->size = max(TXT_WRITE_BUFFER_SIZE, str.size);
        buffer->buf = push_array(arena, u8, buffer->size);

        // Consider this insert as a large paste or file open.
        if (buffer->size > TXT_WRITE_BUFFER_SIZE) {
            u64 newline_count = 0;

            // TODO(fede): Investigate and support CRLF, LF, and CR modes
            for (u64 i = 0; i < str.size; i++) {
                if (str.str[i] == '\n') {
                    newline_count++;
                }
            }

            buffer->line_starts = push_array(arena, u64, newline_count);
        } else {
            buffer->line_starts = push_array(arena, u64, TXT_WRITE_BUFFER_MAX_LINES);
        }

        // NOTE(fede): The result contents are not updated
        buffer->line_count = 1; 
    }

    return &buffer_n->v;
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
    u64 r_size = piece->size - at - gap;

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
    assert(str.size);

    TXT_Buffer *buffer = txt_get_buffer_for_str(arena, text, str);

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

    for (u64 i = 0; i < str.size; i++) {
        // NOTE(fede): Set the line idx to the last inserted byte line idx, if 
        // the last byte is a '\n', we don't want it to have the end.line_idx be
        // on the next line. This reassures us of inclusive ranges, not exclusive.
        end.line_idx = buffer->line_count - 1;

        u64 buf_idx = i + buffer->count;
        if (str.str[i] == '\n') {
            // The next byte is the start of a line
            buffer->line_starts[buffer->line_count] = buf_idx + 1;
            buffer->line_count++;

            piece_newline_count++;
        }

        // STUDY perf
        buffer->buf[buf_idx] = str.str[i];
    }

    buffer->count += str.size;

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
    //
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
    TXT_PieceNode *gap = txt_split_piece_n(arena, text, node, at, n);

    assert(node);
    assert(n);

    TXT_Piece *piece = &node->v;
    assert(at < piece->size);

    u64 n_deleted = n;
    if (piece->size - at - n <= 0) {
        n_deleted = piece->size - at;
    }

    return n_deleted;
}

internal void txt_delete(Arena *arena, TXT_Text *text, u64 at, u64 n) {
    u64 offset = at;
    TXT_PieceNode *delete_n = text->first;
    for (; delete_n != 0; delete_n = delete_n->next) {
        if (delete_n->v.size > offset)
            break;
        offset -= delete_n->v.size;
    }

    u64 n_deleted = 0;
    for (; delete_n != 0; delete_n = delete_n->next) {
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
     * TODO(fede): I have a brute approx, now i have to increase the result by 
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
    for (; line_idx < row; line_idx++) {
        assert(buf_line.line_idx < buffer->line_count);

        // NOTE(fede): if line_idx needs to be incremented, then the next one 
        //      must be available.
        assert(buf_line.line_idx + 1 < buffer->line_count);

        u64 buffer_line_size = 
            buffer->line_starts[buf_line.line_idx + 1] - 
            buffer->line_starts[buf_line.line_idx];
        buffer_line_size -= buf_line.offset; 

        result += buffer_line_size;

        buf_line.line_idx++;
        buf_line.offset = 0;
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
internal String8 txt_get_line(Arena *arena, TXT_Text *text, u64 row) {
    u64 line_idx = 0;
    TXT_PieceNode *piece_n = text->first;

    // NOTE(fede): Get the LinePos of the start of the `row` 
    TXT_LinePos line_start_pos = {0}; 
    {
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
        for (; line_idx < row; line_idx++) {
            if (line_start_pos.line_idx >= buffer->line_count) {
                assert(line_start_pos.line_idx == buffer->line_count);
                break;
            }

            line_start_pos.line_idx++;
            line_start_pos.offset = 0;
        }

        assert(line_start_pos.line_idx < piece->end.line_idx ||
                line_start_pos.line_idx == piece->end.line_idx &&
                line_start_pos.offset <= piece->end.offset);
    }


    // NOTE(fede): Do the string cats
    String8 result = S("");
    {
        TXT_Piece *piece = &piece_n->v;
        TXT_Buffer *buffer = piece->buffer;

        // TODO(fede): Use scratch arena and implement pop
        while (true) {
            TXT_LinePos line_end_pos = piece->end;
            if (line_start_pos.line_idx < line_end_pos.line_idx) {
                line_end_pos.offset =
                    buffer->line_starts[line_start_pos.line_idx + 1] -
                    buffer->line_starts[line_start_pos.line_idx] - 1;
                line_end_pos.line_idx = line_start_pos.line_idx;
            }

            String8 buf_substr = 
                txt_get_buffer_substr(buffer, line_start_pos, line_end_pos);

            result = str8_cat(arena, result, buf_substr);

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

    return result;
}
