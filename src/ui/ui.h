#ifndef UI_H
#define UI_H

typedef enum {
    UI_Axis2_X,
    UI_Axis2_Y,
    
    UI_Axis2_Count,
} UI_Axis2;

typedef u32 UI_BoxFlags; 
enum {
    UI_BoxFlag_Clickable       = (1 << 0), 
    UI_BoxFlag_DrawText        = (1 << 1),
    UI_BoxFlag_Draggable       = (1 << 2),
    // TODO: Hoverable

    UI_BoxFlag_DrawBackground  = (1 << 3),
    UI_BoxFlag_DrawBorder      = (1 << 4),
    UI_BoxFlag_ClipChildren    = (1 << 5),

    // TODO: Impl layout
    UI_BoxFlag_FloatAxis       = (1 << 6),
    UI_BoxFlag_FloatX          = (UI_BoxFlag_FloatAxis << UI_Axis2_X),
    UI_BoxFlag_FloatY          = (UI_BoxFlag_FloatAxis << UI_Axis2_Y),

    UI_BoxFlag_OverflowX       = (1 << 8),
    UI_BoxFlag_OverflowY       = (1 << 9),

    UI_BoxFlag_DrawHotEffects    = (1 << 10),
    UI_BoxFlag_DrawActiveEffects = (1 << 11),

    UI_BoxFlag_HotAnimation    = (1 << 12),
    UI_BoxFlag_ActiveAnimation = (1 << 13),

    UI_BoxFlag_RenderBucket    = (1 << 14),
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
    FC_GlyphRun *display_run_;

    R_Bucket *r_bucket;

    // NOTE(fede): Style stacks
    UI_Size semantic_size[UI_Axis2_Count];
    UI_Axis2 child_layout_axis;
    v4 background_color;
    v4 text_color;
    v4 border_color;
    f32 corner_radius;
    f32 border_thickness;

    FP_Handle font_handle;
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

    v2 mouse_pos;
    v2 rel_mouse_pos;
    v2 drag_delta;

    bool clicked; 
    bool dragging;
    bool hovering;
    bool pressed;
    bool released;

    // bool double_clicked; 
    // bool right_clicked; 
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
    f32 dt;
    
    UI_Box *first_free_box;

    u32 n_boxes;
    u32 box_table_size;
    UI_BoxHashSlot *box_table;

    UI_Key hot;
    UI_Key active;

    UI_Box *root;

    // UI_Box *parent;

    // TODO(fede): Move to an event list maybe?
    //      This would amortize adding left + right mouse presses and other 
    //      stuff. Maybe keybindings as well if they are here.
    bool mouse_release;
    bool mouse_press;
    v2 mouse_pos;
    v2 mouse_delta;

    v2 mouse_drag_start_pos;
    v2 mouse_drag_start_rel_pos;

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

internal UI_Key ui_nil_key(void);
internal UI_Key ui_key_from_string(String8 string);
internal bool ui_key_match(UI_Key a, UI_Key b);
internal bool ui_box_is_nil(UI_Box *box);

internal UI_Box *ui_box_make(UI_BoxFlags flags, String8 string);
internal UI_Box *ui_box_makef(UI_BoxFlags flags, char *fmt, ...);

internal void ui_box_equip_string(UI_Box *box, String8 string);
internal void ui_box_equip_r_bucket(UI_Box *box, R_Bucket *bucket);
// TODO(fede): Move to style stack
internal void ui_box_equip_child_layout_axis(UI_Box *box, UI_Axis2 axis); 

internal UI_Comm ui_comm_from_box(UI_Box *box);

internal void ui_init(void);
internal void ui_begin_build(v2 window_dim, WMEventList *events, f32 dt);
internal void ui_end_build(void);
internal void ui_layout(void);
internal void ui_render(void);

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Helpers

/// Size
internal inline f32 ui_get_em(f32 v, f32 font_size);
internal inline UI_Size ui_size(UI_SizeKind kind, f32 val, f32 strictness);
#define ui_pct(v, s) ui_size(UI_SizeKind_PercentOfParent, (v), (s))
#define ui_px(v, s)  ui_size(UI_SizeKind_Pixels         , (v), (s))
#define ui_em(v, s)  ui_size(UI_SizeKind_EM             , (v), (s))
#define ui_tc(v, s)  ui_size(UI_SizeKind_TextContent    , (v), (s))
#define ui_cs(s)     ui_size(UI_SizeKind_ChildrenSum    , 0  , (s))

internal inline v4 ui_blend_colors(v4 a, v4 b, f32 t);
internal inline v4 ui_darken_color(v4 color, f32 t);
internal inline v4 ui_lighten_color(v4 color, f32 t);

// Font 
internal inline FC_GlyphRun *ui_get_box_display_run(UI_Box *box);

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Common widgets

internal UI_Comm ui_button(String8 str);
internal void ui_spacer(UI_Size size);
internal UI_Comm ui_slider(f32 *val, f32 min, f32 max, String8 str);
internal UI_Comm ui_f32_slider(f32 *val, f32 min, f32 max, String8 str);

internal UI_Comm ui_text_view(Arena *arena, TXT_View *view, String8 label, f32 text_padding_px, bool selected, f32 line_height);

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

#define UI_Padding(v) DeferLoop(ui_spacer(v), ui_spacer(v))
#define UI_NamedColumn(s) UI_ChildLayoutAxis(UI_Axis2_Y) UI_Parent(ui_box_make(0, s)) 
#define UI_NamedRow(s) UI_ChildLayoutAxis(UI_Axis2_X) UI_Parent(ui_box_make(0, s)) 
#define UI_Column UI_ChildLayoutAxis(UI_Axis2_Y) UI_Parent(ui_box_make(0, S8(""))) 
#define UI_Row UI_ChildLayoutAxis(UI_Axis2_X) UI_Parent(ui_box_make(0, S8(""))) 

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Other Macros

#define RGBA(r, g, b, a) ((v4){r, g, b, a})

#endif // UI_H
