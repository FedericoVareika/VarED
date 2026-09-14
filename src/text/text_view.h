#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

typedef enum {
    TXT_ViewAction_Flag_KeepMark               = (1 << 0),
    TXT_ViewAction_Flag_Delete                 = (1 << 1),
    TXT_ViewAction_Flag_ZeroDeltaWithSelection = (1 << 2),
    TXT_ViewAction_Flag_SetHorizontalAnchor    = (1 << 3),
    TXT_ViewAction_Flag_ScanWords              = (1 << 4),
    TXT_ViewAction_AutoScrollLines             = (1 << 5),
} TXT_ViewAction_Flag;

typedef struct TXT_ViewAction TXT_ViewAction;
struct TXT_ViewAction {
    u32 flags;

    i32 row_delta;
    i32 hor_delta;
    f32 hor_anchor_em;

    u32 codepoint;
};

typedef struct TXT_ViewActionNode TXT_ViewActionNode;
struct TXT_ViewActionNode {
    TXT_ViewActionNode *next;
    TXT_ViewAction v;
};

typedef struct TXT_ViewOp TXT_ViewOp;
struct TXT_ViewOp {
    v2u new_cursor;
    v2u new_mark;
    f32 new_hor_anchor_em;
    u32 new_line_offset;

    Rng2u replace_range;
    String8 insert_text;

    bool update_cursor_col;
    bool keep_mark;
};

typedef struct TXT_View TXT_View;
struct TXT_View {
    String8 label;

    Arena *text_arena;

    bool single_line;
    bool file_view;

    TXT_Text *text;
    u64 line_offset;
    f32 sub_line_offset; // NOTE(fede): Positive, between 0 and 1.

    v2u cursor;
    v2u mark;

    f32 horizontal_anchor_em;

    // NOTE(fede): Calculated each frame
    Rect2 text_rect;
    u32 first_line; 
    u32 last_line;

    // NOTE(fede): Per-frame actions.
    TXT_ViewActionNode *first_action;
    TXT_ViewActionNode *last_action;
};

typedef struct TXT_ViewNode TXT_ViewNode;
struct TXT_ViewNode {
    TXT_ViewNode *next;
    TXT_ViewNode *prev;

    TXT_View v;
};

internal TXT_ViewOp txt_op_from_view_action(Arena *arena, TXT_View *view, TXT_ViewAction action);

#endif // TEXT_VIEW_H
