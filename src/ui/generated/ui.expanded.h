////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Styles
////////////////////////////////////////////////////////////////////////////////
/*  
 *  Options: 
 *      - UI_STYLE_STACK_STRUCTS
 *      - UI_STYLE_STACK_HEADERS
 *      - UI_STYLE_STACK_DECLS
 *      - UI_STYLE_STACK_INIT_DEFAULTS
 *      - UI_STYLE_STACK_INIT_BOX
 */
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Structs
/*
*/ typedef struct PrefWidth_Node PrefWidth_Node; struct PrefWidth_Node { PrefWidth_Node *next; UI_Size v; }; /*
*/ typedef struct PrefHeight_Node PrefHeight_Node; struct PrefHeight_Node { PrefHeight_Node *next; UI_Size v; }; /*
*/ typedef struct ChildLayoutAxis_Node ChildLayoutAxis_Node; struct ChildLayoutAxis_Node { ChildLayoutAxis_Node *next; UI_Axis2 v; }; /*

*/ typedef struct BackgroundColor_Node BackgroundColor_Node; struct BackgroundColor_Node { BackgroundColor_Node *next; v4 v; }; /*
*/ typedef struct TextColor_Node TextColor_Node; struct TextColor_Node { TextColor_Node *next; v4 v; }; /*
*/ typedef struct BorderColor_Node BorderColor_Node; struct BorderColor_Node { BorderColor_Node *next; v4 v; }; /*

*/ typedef struct FontHandle_Node FontHandle_Node; struct FontHandle_Node { FontHandle_Node *next; FP_FontHandle v; }; /*
*/ typedef struct FontSize_Node FontSize_Node; struct FontSize_Node { FontSize_Node *next; f32 v; }; /*

*/ typedef struct CornerRadius_Node CornerRadius_Node; struct CornerRadius_Node { CornerRadius_Node *next; f32 v; }; /*
*/ typedef struct BorderThickness_Node BorderThickness_Node; struct BorderThickness_Node { BorderThickness_Node *next; f32 v; }; /*
*/
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Headers
/*
*/ internal void ui_push_pref_width(UI_Size v); /*
*/ internal void ui_push_pref_height(UI_Size v); /*
*/ internal void ui_push_child_layout_axis(UI_Axis2 v); /*

*/ internal void ui_push_background_color(v4 v); /*
*/ internal void ui_push_text_color(v4 v); /*
*/ internal void ui_push_border_color(v4 v); /*

*/ internal void ui_push_font_handle(FP_FontHandle v); /*
*/ internal void ui_push_font_size(f32 v); /*

*/ internal void ui_push_corner_radius(f32 v); /*
*/ internal void ui_push_border_thickness(f32 v); /*
*/
/*
*/ internal void ui_pop_pref_width(void); /*
*/ internal void ui_pop_pref_height(void); /*
*/ internal void ui_pop_child_layout_axis(void); /*

*/ internal void ui_pop_background_color(void); /*
*/ internal void ui_pop_text_color(void); /*
*/ internal void ui_pop_border_color(void); /*

*/ internal void ui_pop_font_handle(void); /*
*/ internal void ui_pop_font_size(void); /*

*/ internal void ui_pop_corner_radius(void); /*
*/ internal void ui_pop_border_thickness(void); /*
*/
/*
*/ internal UI_Size ui_top_pref_width(void); /*
*/ internal UI_Size ui_top_pref_height(void); /*
*/ internal UI_Axis2 ui_top_child_layout_axis(void); /*

*/ internal v4 ui_top_background_color(void); /*
*/ internal v4 ui_top_text_color(void); /*
*/ internal v4 ui_top_border_color(void); /*

*/ internal FP_FontHandle ui_top_font_handle(void); /*
*/ internal f32 ui_top_font_size(void); /*

*/ internal f32 ui_top_corner_radius(void); /*
*/ internal f32 ui_top_border_thickness(void); /*
*/
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Decls
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): State init defaults
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Box make init
///     - Assumes result is the UI_Box
////////////////////////////////////////////////////////////////////////////////
