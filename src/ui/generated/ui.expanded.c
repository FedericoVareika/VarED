////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Styles
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Bodies
/*
*/ internal void ui_push_pref_width(UI_Size v) { PrefWidth_Node *n = push_struct(ui_state->build_arena, PrefWidth_Node); n->v = v; n->next = ui_state->pref_width; ui_state->pref_width = n; } /*
*/ internal void ui_push_pref_height(UI_Size v) { PrefHeight_Node *n = push_struct(ui_state->build_arena, PrefHeight_Node); n->v = v; n->next = ui_state->pref_height; ui_state->pref_height = n; } /*
*/ internal void ui_push_child_layout_axis(UI_Axis2 v) { ChildLayoutAxis_Node *n = push_struct(ui_state->build_arena, ChildLayoutAxis_Node); n->v = v; n->next = ui_state->child_layout_axis; ui_state->child_layout_axis = n; } /*

*/ internal void ui_push_background_color(v4 v) { BackgroundColor_Node *n = push_struct(ui_state->build_arena, BackgroundColor_Node); n->v = v; n->next = ui_state->background_color; ui_state->background_color = n; } /*
*/ internal void ui_push_border_color(v4 v) { BorderColor_Node *n = push_struct(ui_state->build_arena, BorderColor_Node); n->v = v; n->next = ui_state->border_color; ui_state->border_color = n; } /*
*/
/*
*/ internal void ui_pop_pref_width(void) { ui_state->pref_width = ui_state->pref_width->next; } /*
*/ internal void ui_pop_pref_height(void) { ui_state->pref_height = ui_state->pref_height->next; } /*
*/ internal void ui_pop_child_layout_axis(void) { ui_state->child_layout_axis = ui_state->child_layout_axis->next; } /*

*/ internal void ui_pop_background_color(void) { ui_state->background_color = ui_state->background_color->next; } /*
*/ internal void ui_pop_border_color(void) { ui_state->border_color = ui_state->border_color->next; } /*
*/
////////////////////////////////////////////////////////////////////////////////
