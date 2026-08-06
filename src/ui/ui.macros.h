////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Styles

#define STYLE_STACK_DEFS(M) /*
*/ M(PrefWidth       , pref_width        , UI_Size  , semantic_size[UI_Axis2_X] , ui_pct(1)) /*
*/ M(PrefHeight      , pref_height       , UI_Size  , semantic_size[UI_Axis2_Y] , ui_pct(1)) /*
*/ M(ChildLayoutAxis , child_layout_axis , UI_Axis2 , child_layout_axis         , UI_Axis2_Y) /*

*/ M(BackgroundColor , background_color , v4 , background_color , ((v4){0.5 , 0.5 , 0.5 , 1})) /*
*/ M(BorderColor     , border_color     , v4 , border_color     , ((v4){0.5 , 0.5 , 0.5 , 1})) /*
*/

////////////////////////////////////////////////////////////////////////////////

#ifdef MACROS_H

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

#define DEFINE_STYLE_STRUCT(struct_name, name, value_type, box_field, default) \
    typedef struct struct_name##_Node struct_name##_Node; \
    struct struct_name##_Node { \
      struct_name##_Node *next; \
      value_type v; \
    }; 

STYLE_STACK_DEFS(DEFINE_STYLE_STRUCT)

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Headers
  
#define DEFINE_STYLE_HEADERS_PUSH(struct_name, name, value_type, box_field, default) \
    internal void ui_push_##name(value_type v); 

#define DEFINE_STYLE_HEADERS_POP(struct_name, name, value_type, box_field, default) \
    internal void ui_pop_##name(void);

STYLE_STACK_DEFS(DEFINE_STYLE_HEADERS_PUSH)
STYLE_STACK_DEFS(DEFINE_STYLE_HEADERS_POP)

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Decls

#define DEFINE_STYLE_DECLS(struct_name, name, value_type, box_field, default) \
    struct_name##_Node *name;

#define GENERATE_STYLE_DECLS() STYLE_STACK_DEFS(DEFINE_STYLE_DECLS)
    
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): State init defaults

#define DEFINE_STYLE_INIT_DEFAULTS(struct_name, name, value_type, box_field, default) \
    ui_push_##name(default);

#define GENERATE_STYLE_INIT_DEFAULTS() STYLE_STACK_DEFS(DEFINE_STYLE_INIT_DEFAULTS)

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Box make init
///     - Assumes result is the UI_Box

#define DEFINE_STYLE_INIT_BOX(struct_name, name, value_type, box_field, default) \
    result->box_field = ui_state->name->v;   

#define GENERATE_STYLE_INIT_BOX() STYLE_STACK_DEFS(DEFINE_STYLE_INIT_BOX)

////////////////////////////////////////////////////////////////////////////////

#endif
