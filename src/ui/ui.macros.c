#include "ui.macros.h"

#ifdef MACROS_C

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Bodies

#define DEFINE_STYLE_BODIES_PUSH(struct_name, name, value_type, box_field, default) \
    internal void ui_push_##name(value_type v) { \
        struct_name##_Node *n = push_struct(ui_state->build_arena, struct_name##_Node); \
        n->v = v; n->next = ui_state->name; ui_state->name = n; \
    }

#define DEFINE_STYLE_BODIES_POP(struct_name, name, value_type, box_field, default) \
    internal void ui_pop_##name(void) { \
        ui_state->name = ui_state->name->next; \
    }

STYLE_STACK_DEFS(DEFINE_STYLE_BODIES_PUSH)
STYLE_STACK_DEFS(DEFINE_STYLE_BODIES_POP)

////////////////////////////////////////////////////////////////////////////////

#endif
