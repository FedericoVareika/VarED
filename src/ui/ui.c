
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
    // ui_state->parent = &ui_nil_box;

    ui_state->box_table_size = 4096;
    ui_state->box_table = push_array(arena, UI_BoxHashSlot, ui_state->box_table_size);
}

internal String8 ui_hash_string(String8 string) {
    // TODO(fede): Use ### separation if we need it.
    return string;
}

internal String8 ui_display_string(String8 string) {
    // TODO(fede): Display until ##
    return string;
}

internal UI_Key ui_nil_key(void) {
    return (UI_Key){0}; 
}

internal UI_Key ui_key_from_string(String8 string) {
    u64 v = str8_hash_u64(string);
    return (UI_Key){v}; 
}

internal UI_Key ui_key_from_string_seed(String8 string, UI_Key key_seed) {
    UI_Key result = {0};
    if (string.size)
        result.v = str8_hash_u64_seed(string, key_seed.v);
    return result;
}

internal bool ui_key_match(UI_Key a, UI_Key b) {
    return a.v == b.v;
}

internal bool ui_box_is_nil(UI_Box *box) {
    return box == 0 || box == &ui_nil_box;
} 

internal UI_Box *ui_key_is_duplicate(UI_Box *box, UI_Key key) {
    if (ui_box_is_nil(box))
        return 0;

    if (ui_key_match(key, box->key)) {
        return box;
    }

    UI_Box *next = ui_key_is_duplicate(box->next, key);
    UI_Box *first = ui_key_is_duplicate(box->first, key);

    if (next) {
        return next;
    }
    
    if (first) {
        return first;
    }

    return 0;
}
    
internal UI_Box *ui_box_from_key(UI_BoxFlags flags, UI_Key key) {
    UI_Box *result = 0;

    if (ui_key_match(ui_nil_key(), key)) {
        result = push_struct(ui_state->build_arena, UI_Box);
        result->key = key;
    } else {
        UI_BoxHashSlot *slot = &ui_state->box_table[key.v % ui_state->box_table_size];
        for (UI_Box *box = slot->hash_first; !ui_box_is_nil(box); box = box->hash_next) {
            if (ui_key_match(box->key, key)) {
                result = box;
                break;
            }
        }
    }

#if VARED_INTERNAL
    if (!ui_key_match(ui_nil_key(), key)) {
        UI_Box *duplicate_box = ui_key_is_duplicate(ui_state->root, key);
        assert(duplicate_box == 0);
    }
#endif

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

    result->flags = flags;

    result->last_frame_touched_idx = ui_state->frame_idx;

    GENERATE_STYLE_INIT_BOX();

    if (!ui_box_is_nil(result->parent)) {
        DLL_PushBack_nil(result->parent->first, result->parent->last, result, &ui_nil_box);
    }

    ui_state->n_boxes++;

    return result;
} 

internal UI_Box *ui_box_make(UI_BoxFlags flags, String8 string) {
    String8 hash_string = ui_hash_string(string);
    UI_Key key = {0};

    UI_Key seed_key = ui_nil_key();
    for (UI_Box *ancestor = ui_top_parent();
            !ui_box_is_nil(ancestor); 
            ancestor = ancestor->parent) {
        if (!ui_key_match(ui_nil_key(), ancestor->key)) {
            seed_key = ancestor->key;
            break;
        }
    }

    key = ui_key_from_string_seed(hash_string, seed_key);
    UI_Box *result = ui_box_from_key(flags, key);
    result->string = string;
    ui_box_equip_string(result, string);

    return result;
}

// TODO
internal UI_Box *ui_box_makef(UI_BoxFlags flags, char *fmt, ...) {
    // TODO(fede): Somehting like this
    // String8 string = st8_from_fmt(fmt, ...); 
    // return ui_box_make(flags, string);
    String8 str = str8_from_cstr(fmt);

    return ui_box_make(flags, str);
}

internal void ui_box_equip_string(UI_Box *box, String8 string) {
    // box->string = string;

    if (!!(box->flags & UI_BoxFlag_DrawText)) {
        box->display_string = ui_display_string(string);
        ui_get_box_display_run(box);
    }
}

internal void ui_box_equip_r_bucket(UI_Box *box, R_Bucket *bucket) {
    box->r_bucket = bucket;
    box->flags |= UI_BoxFlag_RenderBucket;
}

internal void ui_box_equip_child_layout_axis(UI_Box *box, UI_Axis2 axis) {
    box->child_layout_axis = axis;
}

internal UI_Comm ui_comm_from_box(UI_Box *box) {
    UI_Comm comm = { .box = box };
    comm.mouse_pos = ui_state->mouse_pos;
    comm.rel_mouse_pos = v2_sub(comm.mouse_pos, box->rect.min);

    bool mouse_interactable = box->flags & UI_BoxFlag_Clickable || 
        box->flags & UI_BoxFlag_Draggable;

    if (mouse_interactable) {
        bool mouse_inside_box = rect2_test_inside(box->rect, ui_state->mouse_pos);
        bool mouse_press = ui_state->mouse_press;
        bool mouse_release = ui_state->mouse_release;

        bool hot = ui_key_match(ui_state->hot, box->key);
        bool active = ui_key_match(ui_state->active, box->key);

        bool hot_change = false;
        bool active_change = false;

        if (hot && !mouse_inside_box) {
            hot = false;
            hot_change = true;
        }

        if (!hot && mouse_inside_box) {
            hot = true;
            hot_change = true;
        }

        if (hot && mouse_press) {
            assert(mouse_inside_box);
            active = true;
            active_change = true;
        }

        // STUDY(fede): Maybe hovering should occurr after some time being hot.
        // if (hot) {
        //     comm.hovering = true;
        // }

        if (active && mouse_release) {
            if (mouse_inside_box) {
                if (box->flags & UI_BoxFlag_Clickable) {
                    comm.clicked = true;
                }
            } else {
                hot = false;
                hot_change = true;
            }

            active = false;
            active_change = true;
        }

        if (box->flags & UI_BoxFlag_Draggable && active) {
            if (active_change) {
                ui_state->mouse_drag_start_pos = ui_state->mouse_pos;
                ui_state->mouse_drag_start_rel_pos = v2_sub(ui_state->mouse_pos, box->rect.min);
            }

            comm.dragging = true;
            comm.drag_delta = v2_sub(ui_state->mouse_pos, ui_state->mouse_drag_start_pos);
        }

        // TODO(fede): Anim
        if (hot_change) {
            ui_state->hot = hot ? box->key : ui_nil_key();
        } 

        if (active_change) {
            ui_state->active = active ? box->key : ui_nil_key();

            if (!active) {
                comm.released = true;
            }
        }

        // Hot anim
        {
            if (hot) {
                box->hot_t += ui_state->dt * 2;
                box->hot_t = min(1, box->hot_t);
            }

            if (!hot) {
                box->hot_t -= ui_state->dt * 2;
                box->hot_t = max(0, box->hot_t);
            }
        }

        // Active anim
        {
            if (active) {
                box->active_t += ui_state->dt * 4;
                box->active_t = min(1, box->active_t);
            }

            if (!active) {
                box->active_t -= ui_state->dt * 2;
                box->active_t = max(0, box->active_t);
            }
        }
    }

    return comm;
}

internal void ui_begin_build(v2 window_dim, WMEventList *events, f32 dt) {
    arena_clear(ui_state->build_arena);
    ui_state->n_boxes = 0;

    ui_state->frame_idx++;
    ui_state->dt = dt;

    ui_state->root = &ui_nil_box;

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
        ui_push_parent(ui_state->root);
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
            ui_state->mouse_delta = v2_sub(event->pos, ui_state->mouse_pos); 
            ui_state->mouse_pos = event->pos;
        } break;
        case WMEventKind_Release: {
            if (event->key == WMKey_MOUSELEFT) {
                ui_state->mouse_release = true;
            }
        } break;
        case WMEventKind_Press: {
            if (event->key == WMKey_MOUSELEFT) {
                ui_state->mouse_press = true;
            }
        } break;
        }
    }
}

internal void ui_end_build(void) {
    u32 n_freed = 0;
    for (u32 i = 0; i < ui_state->box_table_size; i++) {
        UI_BoxHashSlot *slot = &ui_state->box_table[i];

        for (UI_Box *box = slot->hash_first;
                !ui_box_is_nil(box);) {
            UI_Box *next_box = box->hash_next;

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
                n_freed++;
            }

            box = next_box;
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
    } else if (size.kind == UI_SizeKind_EM) {
        // STUDY(fede): Is this correct?
        box->computed_size[axis] = ui_get_em(size.value, box->font_size);

    } else if (size.kind == UI_SizeKind_TextContent) {
        assert(axis == UI_Axis2_X);
        FC_GlyphRun *display_run = ui_get_box_display_run(box);
        box->computed_size[axis] = display_run->advance + size.value * 2;
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
            if (box->child_layout_axis == axis) {
                children_sum += child->computed_size[axis];
            } else {
                children_sum = max(children_sum, child->computed_size[axis]);
            }
        }

        box->computed_size[axis] = children_sum;
    }

    ui_layout_descendant_dependant(box->next, axis);
}

internal void ui_layout_resolve_conflicts(UI_Box *box, UI_Axis2 axis) {
    if (ui_box_is_nil(box))
        return;

    f32 box_size = box->computed_size[axis];
    f32 children_size = 0;
    f32 children_max_shrink = 0;


    for (UI_Box *child = box->first;
            !ui_box_is_nil(child);
            child = child->next) {
        f32 size = child->computed_size[axis];
        f32 strictness = child->semantic_size[axis].strictness;
        if (box->child_layout_axis == axis) {
            children_size += size;
            children_max_shrink += size * (1 - strictness);
        } else {
            children_size = max(children_size, size);
        }
    }

    f32 oversize_amount = children_size - box_size;
    if (!!(box->flags & (UI_BoxFlag_OverflowX << axis))) {
        oversize_amount = 0;
    }
    for (UI_Box *child = box->first;
            !ui_box_is_nil(child);
            child = child->next) {
        f32 child_shrink = 0;

        if (oversize_amount > 0) {
            if (box->child_layout_axis == axis) {
                f32 size = child->computed_size[axis];
                f32 strictness = child->semantic_size[axis].strictness;
                child_shrink = size * (1 - strictness);
                child_shrink *= (oversize_amount / children_max_shrink);
            } else {
                child_shrink = max(child->computed_size[axis] - box_size, 0);
            }
        }

        child->computed_size[axis] -= child_shrink;

        ui_layout_resolve_conflicts(child, axis);
    }
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
    TimeFunction;
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
internal void ui_render_boxes(UI_Box *box, Rect2 clip) {
    if (ui_box_is_nil(box))
        return;

    if (!rect2_overlap(box->rect, clip))
        return;

    v4 background = box->background_color;
    R_Rect2DInst *r_inst = r_push_rect2(
            .pos = box->rect,
            .corner_radius = box->corner_radius,
            .edge_softness = 1,
            .border_thickness = 0,
            .clip = clip,
            R_Color4(RGBA(0, 0, 0, 0)));

    if (!!(box->flags & UI_BoxFlag_DrawBackground)) {
        r_inst->color0 = background;
        r_inst->color1 = background;
        r_inst->color2 = background;
        r_inst->color3 = background;
    }

    if (!!(box->flags & UI_BoxFlag_DrawBorder)) {
        r_push_rect2(
                .pos = box->rect,
                .corner_radius = box->corner_radius,
                .edge_softness = 1,
                .border_thickness = box->border_thickness,
                .clip = clip,
                R_Color4(box->border_color));
    }


    bool mouse_interactable = box->flags & UI_BoxFlag_Clickable || 
        box->flags & UI_BoxFlag_Draggable;

    bool hot = mouse_interactable && ui_key_match(ui_state->hot, box->key);
    bool active = mouse_interactable && ui_key_match(ui_state->active, box->key);

    if (hot && box->flags & UI_BoxFlag_DrawHotEffects ||
        active && box->flags & UI_BoxFlag_DrawActiveEffects) {

        bool hot_anim = !!(box->flags & UI_BoxFlag_HotAnimation);
        bool active_anim = !!(box->flags & UI_BoxFlag_ActiveAnimation);

        f32 transition = 0;
        f32 color_t = 0.3;

        v4 debossed_color0 = ui_lighten_color(r_inst->color0, color_t);
        v4 debossed_color1 = ui_darken_color(r_inst->color1, color_t);
        v4 debossed_color2 = ui_lighten_color(r_inst->color2, color_t);
        v4 debossed_color3 = ui_darken_color(r_inst->color3, color_t);  

        v4 embossed_color0 = ui_darken_color(r_inst->color0, color_t);
        v4 embossed_color1 = ui_lighten_color(r_inst->color1, color_t);
        v4 embossed_color2 = ui_darken_color(r_inst->color2, color_t);  
        v4 embossed_color3 = ui_lighten_color(r_inst->color3, color_t);

        if (mouse_interactable) {
            if (hot_anim) {
                if (hot) {
                    transition -= ease_out_quint_f32(box->hot_t);
                } else {
                    transition -= ease_in_expo_f32(box->hot_t);
                }
            } else if (hot) {
                transition = -1;
            }

            if (active_anim) {
                if (active) {
                    transition = ease_out_quint_f32(box->active_t) * 2 - 1;
                } else {
                    transition += ease_in_expo_f32(box->active_t) * 2;
                }
            } else if (active) {
                transition = 1;
            }
        }

        color_t *= transition;

        if (color_t < 0) {
            color_t = -color_t;
            r_inst->color0 = ui_darken_color(background, color_t);
            r_inst->color1 = ui_lighten_color(background, color_t);
            r_inst->color2 = ui_darken_color(background, color_t);  
            r_inst->color3 = ui_lighten_color(background, color_t);
        } else if (color_t > 0) {
            r_inst->color0 = ui_lighten_color(background, color_t);
            r_inst->color1 = ui_darken_color(background, color_t);
            r_inst->color2 = ui_lighten_color(background, color_t);
            r_inst->color3 = ui_darken_color(background, color_t);  
        }  
    }

    if (!!(box->flags & UI_BoxFlag_DrawText)) {
        // TODO(fede): Text alignment, for now, left aligned.
        FC_GlyphRun *display_run = ui_get_box_display_run(box);

        FP_FontMetrics metrics = fp_get_font_metrics(box->font_handle, box->font_size);

        v2 center = (v2) {
            .x = box->rect.min.x + display_run->advance / 2,
            .y = (box->rect.min.y + box->rect.max.y) / 2,
        };

        Rect2 text_rect = rect2_center_dim(
                center,
                (v2){ display_run->advance, metrics.height });
        v2 text_pos = text_rect.min;

        if (box->semantic_size[UI_Axis2_X].kind == UI_SizeKind_TextContent)
            text_pos.x += box->semantic_size[UI_Axis2_X].value;
        text_pos.y += metrics.ascender;

        // TODO font runs do not work anymore when changing font and stuff?
        for (FC_GlyphPtrNode *glyph_ptr_n = display_run->first;
                glyph_ptr_n != 0;
                glyph_ptr_n = glyph_ptr_n->next) {

            FC_Glyph *glyph = glyph_ptr_n->v;

            if (glyph->codepoint != '\n') {
                v2 pos = v2_add(text_pos, (v2){
                    glyph->metrics.bearing_x,
                    -glyph->metrics.bearing_y,
                });

                pos.x = round_f32_to_int(pos.x);
                pos.y = round_f32_to_int(pos.y);

                v2 dim = {
                    glyph->metrics.width,
                    glyph->metrics.height,
                };

                Rect2 glyph_rect = rect2_min_dim(pos, dim);

                if (!rect2_overlap(glyph_rect, clip))
                    break;

                r_push_rect2(
                        .tex = glyph->tex,
                        .pos = glyph_rect,
                        .uv = glyph->uvs,
                        .clip = clip,
                        R_Color4(box->text_color));
            }

            text_pos.x += glyph->metrics.advance;
        }
    }

    if (!!(box->flags & UI_BoxFlag_RenderBucket)) {
        r_feed_top_bucket(box->r_bucket);
    }
    
    ui_render_boxes(box->next, clip);

    Rect2 child_clip = !!(box->flags & (UI_BoxFlag_ClipChildren)) ? box->rect : clip;

    ui_render_boxes(box->first, child_clip);
}

internal void ui_render(void) {
    TimeFunctionBandwidth(ui_state->n_boxes * sizeof(UI_Box));
    ui_render_boxes(ui_state->root, R2_INF);
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Helpers

// Size
internal inline f32 ui_get_em(f32 v, f32 font_size) {
    u32 one_em = (u32)((96.0f / 72.0f) * font_size);
    return v * one_em;
}

internal inline UI_Size ui_size(UI_SizeKind kind, f32 val, f32 strictness) {
    UI_Size result = {0}; 
    result.kind = kind;
    result.value = val;
    result.strictness = strictness;
    return result;
}

internal inline v4 ui_blend_colors(v4 a, v4 b, f32 t) {
    return v4_lerp(a, b, t);
}

internal inline v4 ui_darken_color(v4 color, f32 t) {
    return ui_blend_colors(color, RGBA(0, 0, 0, 1), t);
}

internal inline v4 ui_lighten_color(v4 color, f32 t) {
    return ui_blend_colors(color, RGBA(1, 1, 1, 1), t);
}

// Font 
// TODO(fede): STUDY If the display string is too large, it will crash the app, do 
//      something about it.
internal inline FC_GlyphRun *ui_get_box_display_run(UI_Box *box) {
    FC_GlyphRun *result = fc_get_string_glyph_run(
            box->font_handle,
            box->display_string,
            box->font_size);
    box->display_run_ = result;

    return result;
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Common widgets

internal UI_Comm ui_button(String8 str) {
    UI_Box *box = ui_box_make(
            UI_BoxFlag_Clickable | 
            UI_BoxFlag_DrawText |
            UI_BoxFlag_DrawBorder | 
            UI_BoxFlag_DrawBackground |
            UI_BoxFlag_DrawHotEffects |
            UI_BoxFlag_DrawActiveEffects |
            UI_BoxFlag_ActiveAnimation |
            UI_BoxFlag_HotAnimation,
            str);
    return ui_comm_from_box(box);
}

internal void ui_spacer(UI_Size size) {
    UI_Box *space = ui_box_from_key(0, ui_nil_key());
    // UI_Box *space = ui_box_from_key(UI_BoxFlag_DrawBorder, ui_nil_key());
    UI_Axis2 axis = ui_top_parent()->child_layout_axis;
    space->semantic_size[axis] = size;
}

internal UI_Comm ui_slider(f32 *val, f32 min, f32 max, String8 str) {
    UI_Comm comm = {0};

    UI_ChildLayoutAxis(UI_Axis2_X)
        UI_Parent(ui_box_makef(UI_BoxFlag_DrawBorder, "slider box"))
    {
        UI_Padding(ui_em(1, 0))
            UI_PrefWidth(ui_tc(0, 1)) 
        {
            ui_box_make(UI_BoxFlag_DrawText, str);
        }

        UI_Padding(ui_em(1, 0))
            UI_PrefWidth(ui_pct(1, 0))
            UI_Column
            UI_Padding(ui_pct(1, 0))

            UI_ChildLayoutAxis(UI_Axis2_X)
            UI_PrefHeight(ui_em(1, 1))
            UI_Parent(ui_box_makef(
                        UI_BoxFlag_Draggable |
                        UI_BoxFlag_DrawBorder |
                        UI_BoxFlag_HotAnimation |
                        UI_BoxFlag_ActiveAnimation,
                        "slider"))
            UI_PrefHeight(ui_pct(1, 1))
        {
            comm = ui_comm_from_box(ui_top_parent());

            v2 ui_rect_dim = rect2_dim(ui_top_parent()->rect);

            if (comm.dragging) {
                v2 delta = v2_sub(comm.mouse_pos, ui_top_parent()->rect.min);
                f32 t = delta.x / ui_rect_dim.x;
                t = clamp(t, 0, 1);
                *val = t * (max - min) + min;
            }

            f32 slider_percentage = (*val - min) / (max - min);

            UI_BackgroundColor(RGBA(0.4, 0.5, 0.5, 1))
                UI_PrefWidth(ui_pct(slider_percentage, 0)) 
            {
                ui_box_makef(UI_BoxFlag_DrawBackground, "");
            }

            ui_spacer(ui_pct(1 - slider_percentage, 0));
        }
    }

    return comm;
}

internal UI_Comm ui_checkbox(bool *val, String8 str) {
    UI_Comm comm = {0};

    UI_ChildLayoutAxis(UI_Axis2_X)
        UI_Parent(ui_box_makef(UI_BoxFlag_DrawBorder, ""))
    {
        UI_Padding(ui_em(1, 0))
            UI_PrefWidth(ui_tc(0, 1)) 
        {
            ui_box_make(UI_BoxFlag_DrawText, str);
        }

        UI_Padding(ui_em(1, 0))
            UI_PrefWidth(ui_em(1, 0))
            UI_Column
            UI_Padding(ui_pct(1, 0))

            UI_ChildLayoutAxis(UI_Axis2_X)
            UI_PrefHeight(ui_em(1, 1))
        {
            if (*val) {
                ui_push_background_color(RGBA(0.4, 0.5, 0.5, 1));
            }

            comm = ui_comm_from_box(ui_box_makef(
                        UI_BoxFlag_Clickable |
                        UI_BoxFlag_DrawBorder |
                        UI_BoxFlag_HotAnimation |
                        UI_BoxFlag_ActiveAnimation |
                        (*val ? UI_BoxFlag_DrawBackground : 0), "checkbox"));

            if (*val) {
                ui_pop_background_color();
            }
            if (comm.clicked) {
                *val = !(*val);
            }
        }
    }
}

internal UI_Comm ui_text_view(
        Arena *arena,
        TXT_View *view,
        String8 label,
        f32 text_padding_em,
        bool selected,
        f32 line_height) {
    UI_Comm result = {0};

    TXT_Text *text = view->text;

    ui_push_child_layout_axis(UI_Axis2_Y);
    UI_Box *text_box = ui_box_make(
            UI_BoxFlag_DrawBorder |
            UI_BoxFlag_Clickable |
            UI_BoxFlag_OverflowY |
            UI_BoxFlag_ClipChildren, label);

    result = ui_comm_from_box(text_box);

    f32 estimated_lines_in_box = text_box->rect.max.y - text_box->rect.min.y;
    estimated_lines_in_box /= ui_get_em(line_height, text_box->font_size);
    estimated_lines_in_box += 1;

    v2u64 line_range = {
        .min = view->line_offset + 1,
        .max = view->line_offset + estimated_lines_in_box,
    };

    f32 text_padding_px = ui_get_em(text_padding_em, text_box->font_size);

    UI_Parent(text_box)
        UI_PrefHeight(ui_em(line_height, 1))
        UI_PrefWidth(ui_tc(text_padding_px, 0))
    {
        TimeBlock(S8("UI Build Text"));

        ui_spacer(ui_em(text_padding_em, 0));

        u32 line_idx = 1;
        for (u32 line_num = line_range.min; 
                line_num <= line_range.max && line_idx <= txt_get_n_lines(text);
                line_num++, line_idx++) {
            String8 display_string = txt_get_line(arena, text, line_num);
            String8 key_string = str8_cat(
                    arena,
                    S8("line"),
                    str8_from_u32(arena, line_idx));

            UI_Box *line_box = ui_box_make(
                    UI_BoxFlag_DrawText,
                    key_string); 
            ui_box_equip_string(line_box, display_string);

            UI_Comm line_comm = ui_comm_from_box(line_box);

            if (selected && view->cursor.y == line_num) {
                f32 advance = 0; 
                if (display_string.size) {
                    FC_GlyphRun *glyph_run = ui_get_box_display_run(line_box);
                    u32 bytes_consumed = 0;
                    for (FC_GlyphPtrNode *glyph_ptr_n = glyph_run->first;
                            glyph_ptr_n != 0 && bytes_consumed < view->cursor.x;
                            glyph_ptr_n = glyph_ptr_n->next) {
                        FC_Glyph *glyph = glyph_ptr_n->v;
                        bytes_consumed += utf8_encode(glyph->codepoint, 0);
                        advance += glyph->metrics.advance;
                    }
                }

                R_Bucket *line_bucket = r_get_new_bucket();
                ui_box_equip_r_bucket(line_box, line_bucket);
                line_box->r_bucket = line_bucket;
                r_push_bucket(line_bucket);
                {
                    f32 cursor_width = ui_top_font_size() / 10;
                    Rect2 cursor_rect = (Rect2){
                        .V4 = V4(
                                line_box->rect.min.x + advance + text_padding_px,
                                line_box->rect.min.y,
                                line_box->rect.min.x + advance + cursor_width + text_padding_px,
                                line_box->rect.max.y),
                    };

                    r_push_rect2(.pos = cursor_rect);
                }
                r_pop_bucket();
            }
        }
    }

    ui_pop_child_layout_axis();

    return result;
}
