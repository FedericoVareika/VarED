#ifndef UI_H
#define UI_H

typedef u32 UI_BoxFlags; 
enum {
    UI_BoxFlag_Clickable    = (2 << 0), 
    UI_BoxFlag_DrawText     = (2 << 1),
};

typedef struct UI_Key UI_Key; 
struct UI_Key {
    u64 v;
};

typedef enum {
    UI_SizeKind_Null,

    UI_SizeKind_Pixels,             
    UI_SizeKind_TextContent,        
    UI_SizeKind_EM,
    UI_SizeKind_PercentOfParent,    
    UI_SizeKind_ChildrenSum,        

    UI_SizeKind_Count,
} UI_SizeKind;

typedef enum {
    UI_Axis2_X,
    UI_Axis2_Y,
    
    UI_Axis2_Count,
} UI_Axis2;

typedef struct UI_Size UI_Size;
struct UI_Size {
    UI_SizeKind kind;
    f32 value;
    f32 strictness;
};

typedef struct UI_Box UI_Box;
struct UI_Box {
    UI_Box *first;
    UI_Box *last;
    UI_Box *next;
    UI_Box *prev;
    UI_Box *parent;

    UI_Box *hash_next;
    UI_Box *hash_prev;

    UI_Key key;
    u64 last_frame_touched_idx;

    // NOTE(fede): Per frame
    UI_BoxFlags flags;
    String8 string;

    String8 display_string;
    FC_GlyphRun *display_run;

    // NOTE(fede): Style stacks
    UI_Size semantic_size[UI_Axis2_Count];
    UI_Axis2 child_layout_axis;
    v4 background_color;
    v4 text_color;
    v4 border_color;
    f32 corner_radius;
    f32 border_thickness;

    FP_FontHandle font_handle;
    f32 font_size;

    // NOTE(fede): Computed at layout
    f32 computed_position[UI_Axis2_Count];
    f32 computed_size[UI_Axis2_Count];
    Rect2 rect;

    // // NOTE(fede): Persistent data
    f32 hot_t;
    f32 active_t;
}; 

typedef struct UI_Comm UI_Comm; 
struct UI_Comm {
    UI_Box *box;

    bool clicked; 

    // bool double_clicked; 
    // bool right_clicked; 
    // bool pressed;
    // bool released;
    // bool dragging;
    // bool hovering;
};

typedef struct UI_BoxHashSlot UI_BoxHashSlot;
struct UI_BoxHashSlot {
    UI_Box *hash_first;
    UI_Box *hash_last;
};

#define MACROS_H
#include "ui.macros.h"
#undef MACROS_H

typedef struct UI_State UI_State;
struct UI_State {
    Arena *arena;
    Arena *build_arena;
    u64 frame_idx;
    
    UI_Box *first_free_box;

    u32 box_table_size;
    UI_BoxHashSlot *box_table;

    UI_Key hot;
    UI_Key active;

    UI_Box *root;

    UI_Box *parent;

    // TODO(fede): Move to an event list maybe?
    //      This would amortize adding left + right mouse presses and other 
    //      stuff. Maybe keybindings as well if they are here.
    bool mouse_release;
    bool mouse_press;
    v2 mouse_pos;

    // Style Stack
    GENERATE_STYLE_DECLS()
};

global UI_Box ui_nil_box = {
    .first = &ui_nil_box,
    .last = &ui_nil_box,
    .prev = &ui_nil_box,
    .next = &ui_nil_box,
    .parent = &ui_nil_box,
};

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): API

internal UI_Key ui_key_null(void);
internal UI_Key ui_key_from_string(String8 string);
internal bool ui_key_match(UI_Key a, UI_Key b);
internal bool ui_box_is_nil(UI_Box *box);

internal UI_Box *ui_box_make(UI_BoxFlags flags, String8 string);
internal UI_Box *ui_box_makef(UI_BoxFlags flags, char *fmt, ...);

internal void ui_box_equip_string(UI_Box *box, String8 string);
// TODO(fede): Move to style stack
internal void ui_box_equip_child_layout_axis(UI_Box *box, UI_Axis2 axis); 

internal UI_Box *ui_push_parent(UI_Box *box);
internal UI_Box *ui_pop_parent(void);

internal UI_Comm ui_comm_from_box(UI_Box *box);

internal void ui_init(void);
internal void ui_begin_build(v2 window_dim, WMEventList *events);
internal void ui_end_build(void);
internal void ui_layout(void);
internal void ui_render(void);

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Helpers

/// Size
internal inline UI_Size ui_size(UI_SizeKind kind, f32 val, f32 strictness);
#define ui_pct(v, s) ui_size(UI_SizeKind_PercentOfParent, (v), (s))
#define ui_px(v, s)  ui_size(UI_SizeKind_Pixels         , (v), (s))
#define ui_em(v, s)  ui_size(UI_SizeKind_EM             , (v), (s))
#define ui_tc(v, s)  ui_size(UI_SizeKind_TextContent    , (v), (s))

internal inline v4 ui_darken_color(v4 color, f32 t);
internal inline v4 ui_lighten_color(v4 color, f32 t);

// Font 

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Common widgets

internal UI_Comm ui_button(String8 str);

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Style stack DeferLoops

#define UI_Parent(v) DeferLoop(ui_push_parent((v)), ui_pop_parent())
#define UI_PrefWidth(v) DeferLoop(ui_push_pref_width((v)), ui_pop_pref_width())
#define UI_PrefHeight(v) DeferLoop(ui_push_pref_height((v)), ui_pop_pref_height())
#define UI_ChildLayoutAxis(v) DeferLoop(ui_push_child_layout_axis((v)), ui_pop_child_layout_axis())

#define UI_TextColor(v) DeferLoop(ui_push_text_color((v)), ui_pop_text_color())
#define UI_BackgroundColor(v) DeferLoop(ui_push_background_color((v)), ui_pop_background_color())
#define UI_BorderColor(v) DeferLoop(ui_push_border_color((v)), ui_pop_border_color())

#define UI_Font(v) DeferLoop(ui_push_font_handle(v), ui_pop_font_handle())
#define UI_FontSize(v) DeferLoop(ui_push_font_size(v), ui_pop_font_size())

#define UI_CornerRadius(v) DeferLoop(ui_push_corner_radius(v), ui_pop_corner_radius())
#define UI_BorderThickness(v) DeferLoop(ui_push_border_thickness(v), ui_pop_border_thickness())

// #define UI_x(v) DeferLoop(ui_push_x(v), ui_pop_x())

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Other Macros

#define RGBA(r, g, b, a) ((v4){r, g, b, a})

#endif // UI_H
