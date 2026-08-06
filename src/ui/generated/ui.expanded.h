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
*/ typedef struct BorderColor_Node BorderColor_Node; struct BorderColor_Node { BorderColor_Node *next; v4 v; }; /*
*/
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Headers
/*
*/ internal void ui_push_pref_width(UI_Size v); /*
*/ internal void ui_push_pref_height(UI_Size v); /*
*/ internal void ui_push_child_layout_axis(UI_Axis2 v); /*

*/ internal void ui_push_background_color(v4 v); /*
*/ internal void ui_push_border_color(v4 v); /*
*/
/*
*/ internal void ui_pop_pref_width(void); /*
*/ internal void ui_pop_pref_height(void); /*
*/ internal void ui_pop_child_layout_axis(void); /*

*/ internal void ui_pop_background_color(void); /*
*/ internal void ui_pop_border_color(void); /*
*/
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Decls
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): State init defaults
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Box make init
///     - Assumes result is the UI_Box
////////////////////////////////////////////////////////////////////////////////
