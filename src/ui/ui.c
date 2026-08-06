
global UI_State *ui_state = 0;

#define MACROS_C
#include "ui.macros.c"
#undef MACROS_C

internal void ui_init(void) {
    Arena *arena = arena_alloc();
    ui_state = push_struct(arena, UI_State);
    ui_state->arena = arena;
    ui_state->build_arena = arena_alloc();

    ui_state->frame_idx = 0;

    ui_state->root = &ui_nil_box;
    ui_state->parent = &ui_nil_box;

    ui_state->box_table_size = 100;
    ui_state->box_table = push_array(arena, UI_BoxHashSlot, ui_state->box_table_size);
}

internal String8 ui_hash_string(String8 string) {
    // TODO(fede): Use ### separation if we need it.
    return string;
}

internal UI_Key ui_nil_key(void) {
    return (UI_Key){0}; 
}

internal UI_Key ui_key_from_string(String8 string) {
    u64 v = str8_hash_u64(string);
    return (UI_Key){v}; 
}

internal bool ui_key_match(UI_Key a, UI_Key b) {
    return a.v == b.v;
}

internal bool ui_box_is_nil(UI_Box *box) {
    return box == 0 || box == &ui_nil_box;
} 

internal UI_Box *ui_box_from_key(UI_Key key) {
    UI_BoxHashSlot *slot = &ui_state->box_table[key.v % ui_state->box_table_size];

    for (UI_Box *box = slot->hash_first; !ui_box_is_nil(box); box = box->next) {
        if (ui_key_match(box->key, key))
            return box;
    }

    return 0;
} 

internal UI_Box *ui_box_make(UI_BoxFlags flags, String8 string) {
    String8 hash_string = ui_hash_string(string);
    UI_Key key = ui_key_from_string(hash_string);

    UI_Box *result = ui_box_from_key(key);

    if (!result) {
        if (ui_state->first_free_box) {
            result = ui_state->first_free_box;
            ui_state->first_free_box = ui_state->first_free_box->next;
        } else {
            result = push_struct(ui_state->arena, UI_Box);
            UI_BoxHashSlot *slot = &ui_state->box_table[key.v % ui_state->box_table_size];
            DLL_PushBack_NP_nil(
                    slot->hash_first,
                    slot->hash_last,
                    result,
                    hash_next,
                    hash_prev,
                    &ui_nil_box);
        }

        result->key = key;
    }

    assert(ui_key_match(result->key, key));

    result->first = result->last = result->next = result->prev = result->parent = &ui_nil_box;

    if (!ui_box_is_nil(ui_state->parent)) {
        result->parent = ui_state->parent;
        DLL_PushBack_nil(result->parent->first, result->parent->last, result, &ui_nil_box);
    }

    result->flags = flags;
    result->string = string;

    result->last_frame_touched_idx = ui_state->frame_idx;

    GENERATE_STYLE_INIT_BOX();

    return result;
}

// TODO
internal UI_Box *ui_box_makef(UI_BoxFlags flags, char *fmt, ...) {
    // TODO(fede): Somehting like this
    // String8 string = st8_from_fmt(fmt, ...); 
    // return ui_box_make(flags, string);
    return ui_box_make(flags, str8_from_cstr(fmt));
}

// TODO
internal void ui_box_equip_string(UI_Box *box, String8 string) {
    box->string = string;

    /*
    // STUDY(fede): Dont know if i should update key with this.
    String8 hash_string = ui_hash_string(string);
    box->key = ui_key_from_hash_string(hash_string);
    */
}

internal void ui_box_equip_child_layout_axis(UI_Box *box, UI_Axis2 axis) {
    box->child_layout_axis = axis;
}

internal UI_Box *ui_push_parent(UI_Box *box) {
    ui_state->parent = box;
    return ui_state->parent;
}

internal UI_Box *ui_pop_parent(void) {
    UI_Box *result = ui_state->parent;
    ui_state->parent = result->parent;
    return result;
}

internal UI_Comm ui_comm_from_box(UI_Box *box) {
    UI_Comm comm = { .box = box };

    bool mouse_inside_box = rect2_test_inside(box->rect, ui_state->mouse_pos);

    // Clickable, Hot and not mouse inside -> not hot anymore
    if (box->flags & UI_BoxFlag_Clickable && 
            ui_key_match(ui_state->hot, box->key) &&
            !mouse_inside_box) {
        ui_state->hot = ui_nil_key();
    }

    // Clickable, not hot and mouse hover -> hot
    if (box->flags & UI_BoxFlag_Clickable && 
            !ui_key_match(ui_state->hot, box->key) &&
            mouse_inside_box) {
        ui_state->hot = box->key;
    }

    // Clickable, Hot and Mouse press 
    if (box->flags & UI_BoxFlag_Clickable && 
            ui_key_match(ui_state->hot, box->key) &&
            ui_state->mouse_press) {
        if (mouse_inside_box) {
            ui_state->active = box->key;
        } else {
            assert(!"Why is the box hot when the mouse is not hovering?");
            ui_state->hot = ui_nil_key();
        }
    }

    // Clickable, Active and Mouse release 
    if (box->flags & UI_BoxFlag_Clickable && 
            ui_key_match(ui_state->active, box->key) &&
            ui_state->mouse_release) {
        if (mouse_inside_box) {
            comm.clicked = true;
        } else {
            ui_state->hot = ui_nil_key();
        }

        ui_state->active = ui_nil_key();
    }

    return comm;
}

internal void ui_begin_build(v2 window_dim, WMEventList *events) {
    ui_state->frame_idx++;

    ui_state->root = &ui_nil_box;
    ui_state->parent = &ui_nil_box;

    GENERATE_STYLE_INIT_DEFAULTS()

    {
        UI_Box *root = ui_box_make(0, S8("##__ui_root__"));
        root->semantic_size[UI_Axis2_X] = (UI_Size){
            .kind = UI_SizeKind_Pixels,
            .value = window_dim.x,
        };
        root->semantic_size[UI_Axis2_Y] = (UI_Size){
            .kind = UI_SizeKind_Pixels,
            .value = window_dim.y,
        };

        ui_state->root = root;
        ui_state->parent = ui_state->root;
    }

    ui_state->mouse_press = false;
    ui_state->mouse_release = false;

    for (WMEventNode *event_n = events->first;
            event_n != 0;
            event_n = event_n->next) {
        WMEvent *event = &event_n->v;
        
        // TODO(fede): Handle other key presses and releases
        switch (event->kind) {
        case WMEventKind_MouseMove: {
            ui_state->mouse_pos = event->pos;
        } break;
        case WMEventKind_Release: {
            if (event->key = WMKey_MOUSELEFT) {
                ui_state->mouse_release = true;
            }
        } break;
        case WMEventKind_Press: {
            if (event->key = WMKey_MOUSELEFT) {
                ui_state->mouse_press = true;
            }
        } break;
        }
    }
}

internal void ui_end_build(void) {
    for (u32 i = 0; i < ui_state->box_table_size; i++) {
        UI_BoxHashSlot *slot = &ui_state->box_table[i];

        for (UI_Box *box = slot->hash_first;
                !ui_box_is_nil(box);
                box = box->hash_next) {
            if (box->last_frame_touched_idx != ui_state->frame_idx) {
                DLL_Remove_NP_nil(
                        slot->hash_first, 
                        slot->hash_last, 
                        box,
                        hash_next,
                        hash_prev, 
                        &ui_nil_box);
                box->next = ui_state->first_free_box;
                ui_state->first_free_box = box;
            }
        }
    }
}

/*
 *  Independent sizes (any order)
 *  Sizes dependant on ancestors (pre order)
 *  Sizes dependant on descendants (post order)
 *  Resolve conflicts (pre order)
 *  Calculate positions, rects (pre order)
 *
 * */

internal void ui_layout_independent(UI_Box *box, UI_Axis2 axis) {
    if (ui_box_is_nil(box))
        return;

    UI_Size size = box->semantic_size[axis];
    if (size.kind == UI_SizeKind_Pixels) {
        box->computed_size[axis] = size.value; 
    } else if (size.kind == UI_SizeKind_TextContent) {
        /* TODO
        f32 text_width = fc_get_string_width(box->string)?
        box->computed_size[axis] = text_width;
        */
    }

    ui_layout_independent(box->next, axis);
    ui_layout_independent(box->first, axis);
}

internal void ui_layout_ancestor_dependant(UI_Box *box, UI_Axis2 axis) {
    if (ui_box_is_nil(box))
        return;

    UI_Size size = box->semantic_size[axis];
    if (size.kind == UI_SizeKind_PercentOfParent) {
        assert(!ui_box_is_nil(box->parent));
        f32 parent_computed = box->parent->computed_size[axis];
        
        box->computed_size[axis] = parent_computed * size.value;
    }

    ui_layout_ancestor_dependant(box->first, axis);
    ui_layout_ancestor_dependant(box->next, axis);
}

internal void ui_layout_descendant_dependant(UI_Box *box, UI_Axis2 axis) {
    if (ui_box_is_nil(box))
        return;

    ui_layout_descendant_dependant(box->first, axis);

    UI_Size size = box->semantic_size[axis];
    if (size.kind == UI_SizeKind_ChildrenSum) {
        f32 children_sum = 0;
        for (UI_Box *child = box->first;
                !ui_box_is_nil(child);
                child = child->next) {
            // STUDY(fede): Compare against child_layout_axis?
            children_sum += child->computed_size[axis];
        }

        box->computed_size[axis] = children_sum;
    }

    ui_layout_descendant_dependant(box->next, axis);
}

// TODO
internal void ui_layout_resolve_conflicts(UI_Box *box, UI_Axis2 axis) {
    if (ui_box_is_nil(box))
        return;

    UI_Size axis_size = box->semantic_size[axis];
}

internal void ui_layout_end_calc(UI_Box *box, UI_Axis2 axis, f32 layout_pos) {
    if (ui_box_is_nil(box))
        return;

    for (UI_Box *child = box->first;
            !ui_box_is_nil(child);
            child = child->next) {
        child->computed_position[axis] = layout_pos;   
        child->rect.min.e[axis] = layout_pos;
        child->rect.max.e[axis] = layout_pos + child->computed_size[axis];

        ui_layout_end_calc(child, axis, layout_pos);

        if (box->child_layout_axis == axis) {
            layout_pos += child->computed_size[axis];
        }
    }
}

internal void ui_layout(void) {
    UI_Box *root = ui_state->root;
    for (int axis = 0; axis < UI_Axis2_Count; axis++) {
        ui_layout_independent(root, axis);
        ui_layout_ancestor_dependant(root, axis);
        ui_layout_descendant_dependant(root, axis);
        ui_layout_resolve_conflicts(root, axis);
        ui_layout_end_calc(root, axis, 0);
    }
}

// TODO(fede): Do this correctly 
internal void ui_render_boxes(UI_Box *box) {
    if (ui_box_is_nil(box))
        return;

    v4 white = box->background_color;
    white.a = 1;
    v4 black = box->background_color;
    black.a = 0;
    if (ui_key_match(ui_state->active, box->key)) 
        r_push_rect2(
                .pos = box->rect,
                .color0 = black,
                .color2 = black,
                .color1 = white,
                .color3 = white,
                );
    else if (ui_key_match(ui_state->hot, box->key))
        r_push_rect2(
                .pos = box->rect,
                .color0 = white,
                .color2 = white,
                .color1 = black,
                .color3 = black,
                );
    else 
        r_push_rect2(
                .pos = box->rect,
                .color0 = white,
                .color2 = white,
                .color1 = white,
                .color3 = white,
                );
    
    ui_render_boxes(box->next);
    ui_render_boxes(box->first);
}

internal void ui_render(void) {
    ui_render_boxes(ui_state->root);

    Rect2 mouse_rect = rect2_center_dim(ui_state->mouse_pos, (v2){5, 5});
    r_push_rect2(.pos = mouse_rect);
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Helpers

internal UI_Size ui_pct(f32 val) {
    UI_Size result = {0}; 
    result.kind = UI_SizeKind_PercentOfParent;
    result.value = val;
    return result;
}

internal UI_Size ui_px(f32 val) {
    UI_Size result = {0}; 
    result.kind = UI_SizeKind_Pixels;
    result.value = val;
    return result;
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Common widgets

internal UI_Comm ui_button(String8 str) {
    UI_Box *box = ui_box_make(UI_BoxFlag_Clickable, str);
    return ui_comm_from_box(box);
}

