#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

typedef enum {
    TXT_ViewAction_Flag_KeepMark               = (1 << 0),
    TXT_ViewAction_Flag_Delete                 = (1 << 1),
    TXT_ViewAction_Flag_ZeroDeltaWithSelection = (1 << 2),
    TXT_ViewAction_Flag_KeepBehindInsertion    = (1 << 3),
} TXT_ViewAction_Flag;

typedef struct TXT_ViewAction TXT_ViewAction;
struct TXT_ViewAction {
    u32 flags;

    i32 row_delta;
    i32 hor_char_delta;

    u32 codepoint;
};

typedef struct TXT_ViewOp TXT_ViewOp;
struct TXT_ViewOp {
    v2u new_cursor;
    v2u new_mark;

    Rng2u replace_range;
    String8 insert_text;

    bool update_cursor_col;
};

typedef struct TXT_View TXT_View;
struct TXT_View {
    String8 label;

    Arena *text_arena;

    bool single_line;
    bool file_view;

    TXT_Text *text;
    u64 line_offset;

    v2u cursor;
    v2u mark;

    bool anchor_changed;
    f32 horizontal_anchor_em;

    // NOTE(fede): Per-frame actions.
    TXT_ViewAction action;
};

typedef struct TXT_ViewNode TXT_ViewNode;
struct TXT_ViewNode {
    TXT_ViewNode *next;
    TXT_ViewNode *prev;

    TXT_View v;
};

internal TXT_ViewOp txt_op_from_view_action(Arena *arena, TXT_View *view, TXT_ViewAction action);

#endif // TEXT_VIEW_H
