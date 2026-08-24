#ifndef TEXT_H
#define TEXT_H

#define TXT_WRITE_BUFFER_SIZE kilobytes(4)
#define TXT_WRITE_BUFFER_MAX_LINES 1000

typedef struct TXT_Buffer TXT_Buffer;
struct TXT_Buffer {
    u8 *buf;
    u64 size;
    u64 count;

    u64 *line_starts;
    u64 line_count;  
};

typedef struct TXT_BufferNode TXT_BufferNode;
struct TXT_BufferNode {
    TXT_BufferNode *next;

    TXT_Buffer v;
};

typedef struct TXT_LinePos TXT_LinePos;
struct TXT_LinePos {
    u64 line_idx;
    u64 offset;
};

typedef struct TXT_Piece TXT_Piece;
struct TXT_Piece {
    TXT_Buffer *buffer;
    TXT_LinePos start;
    TXT_LinePos end;

    u64 size;
    u64 newline_count;
};

typedef struct TXT_PieceNode TXT_PieceNode;
struct TXT_PieceNode {
    TXT_PieceNode *next;
    TXT_PieceNode *prev;

    TXT_Piece v;
};

typedef struct TXT_Text TXT_Text;
struct TXT_Text {
    TXT_BufferNode *buffer_queue;

    TXT_PieceNode *first_free_piece;
    TXT_PieceNode *first;
    TXT_PieceNode *last;
};

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Helpers

internal TXT_PieceNode *txt_get_piece_n(Arena *arena, TXT_Text *text);
internal void txt_remove_piece_n(TXT_Text *text ,TXT_PieceNode *piece_n);

internal inline u64 txt_piece_line_start_count(TXT_Piece *piece);

internal TXT_LinePos txt_advance_line_pos(TXT_Buffer *buffer, TXT_LinePos pos, u64 n);
internal TXT_Buffer *txt_get_buffer_for_str(Arena *arena, TXT_Text *text, String8 str);

internal bool txt_line_pos_is_end_of_line(TXT_Buffer *buffer, TXT_LinePos pos);
internal inline bool txt_line_pos_is_continuation(TXT_Buffer *buffer, TXT_LinePos a, TXT_LinePos b);

internal TXT_PieceNode *txt_split_piece_n(Arena *arena, TXT_Text *text, TXT_PieceNode *piece_n, u64 at, u64 gap);
internal u64 txt_delete_from_node(Arena *arena, TXT_Text *text, TXT_PieceNode *node, u64 at, u64 n);

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Text Modify API

internal void txt_insert(Arena *arena, TXT_Text *text, String8 str, u64 at);
internal void txt_delete(Arena *arena, TXT_Text *text, u64 at, u64 n);

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Text Query API

internal u64 txt_get_n_lines(TXT_Text *text);
internal u64 txt_get_line_offset(TXT_Text *text, u64 row);
internal String8 txt_get_line(Arena *arena, TXT_Text *text, u64 row);

#endif // TEXT_H
