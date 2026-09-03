#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

typedef struct TXT_View TXT_View;
struct TXT_View {
    String8 label;

    Arena *text_arena;

    bool single_line;
    TXT_Text *text;

    u32 cursor_row;
    u32 cursor_col;
};

typedef struct TXT_ViewNode TXT_ViewNode;
struct TXT_ViewNode {
    TXT_ViewNode *next;
    TXT_ViewNode *prev;

    TXT_View v;
};

#endif // TEXT_VIEW_H
