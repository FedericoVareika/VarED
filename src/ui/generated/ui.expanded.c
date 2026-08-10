////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Styles
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Bodies
/*
*/ internal void ui_push_parent(UI_Box * v) { Parent_Node *n = push_struct(ui_state->build_arena, Parent_Node); n->v = v; n->next = ui_state->parent; ui_state->parent = n; } /*
*/ internal void ui_push_pref_width(UI_Size v) { PrefWidth_Node *n = push_struct(ui_state->build_arena, PrefWidth_Node); n->v = v; n->next = ui_state->pref_width; ui_state->pref_width = n; } /*
*/ internal void ui_push_pref_height(UI_Size v) { PrefHeight_Node *n = push_struct(ui_state->build_arena, PrefHeight_Node); n->v = v; n->next = ui_state->pref_height; ui_state->pref_height = n; } /*
*/ internal void ui_push_child_layout_axis(UI_Axis2 v) { ChildLayoutAxis_Node *n = push_struct(ui_state->build_arena, ChildLayoutAxis_Node); n->v = v; n->next = ui_state->child_layout_axis; ui_state->child_layout_axis = n; } /*

*/ internal void ui_push_background_color(v4 v) { BackgroundColor_Node *n = push_struct(ui_state->build_arena, BackgroundColor_Node); n->v = v; n->next = ui_state->background_color; ui_state->background_color = n; } /*
*/ internal void ui_push_text_color(v4 v) { TextColor_Node *n = push_struct(ui_state->build_arena, TextColor_Node); n->v = v; n->next = ui_state->text_color; ui_state->text_color = n; } /*
*/ internal void ui_push_border_color(v4 v) { BorderColor_Node *n = push_struct(ui_state->build_arena, BorderColor_Node); n->v = v; n->next = ui_state->border_color; ui_state->border_color = n; } /*

*/ internal void ui_push_font_handle(FP_Handle v) { FontHandle_Node *n = push_struct(ui_state->build_arena, FontHandle_Node); n->v = v; n->next = ui_state->font_handle; ui_state->font_handle = n; } /*
*/ internal void ui_push_font_size(f32 v) { FontSize_Node *n = push_struct(ui_state->build_arena, FontSize_Node); n->v = v; n->next = ui_state->font_size; ui_state->font_size = n; } /*

*/ internal void ui_push_corner_radius(f32 v) { CornerRadius_Node *n = push_struct(ui_state->build_arena, CornerRadius_Node); n->v = v; n->next = ui_state->corner_radius; ui_state->corner_radius = n; } /*
*/ internal void ui_push_border_thickness(f32 v) { BorderThickness_Node *n = push_struct(ui_state->build_arena, BorderThickness_Node); n->v = v; n->next = ui_state->border_thickness; ui_state->border_thickness = n; } /*
*/
/*
*/ internal void ui_pop_parent(void) { ui_state->parent = ui_state->parent->next; } /*
*/ internal void ui_pop_pref_width(void) { ui_state->pref_width = ui_state->pref_width->next; } /*
*/ internal void ui_pop_pref_height(void) { ui_state->pref_height = ui_state->pref_height->next; } /*
*/ internal void ui_pop_child_layout_axis(void) { ui_state->child_layout_axis = ui_state->child_layout_axis->next; } /*

*/ internal void ui_pop_background_color(void) { ui_state->background_color = ui_state->background_color->next; } /*
*/ internal void ui_pop_text_color(void) { ui_state->text_color = ui_state->text_color->next; } /*
*/ internal void ui_pop_border_color(void) { ui_state->border_color = ui_state->border_color->next; } /*

*/ internal void ui_pop_font_handle(void) { ui_state->font_handle = ui_state->font_handle->next; } /*
*/ internal void ui_pop_font_size(void) { ui_state->font_size = ui_state->font_size->next; } /*

*/ internal void ui_pop_corner_radius(void) { ui_state->corner_radius = ui_state->corner_radius->next; } /*
*/ internal void ui_pop_border_thickness(void) { ui_state->border_thickness = ui_state->border_thickness->next; } /*
*/
/*
*/ internal UI_Box * ui_top_parent(void) { return ui_state->parent->v; } /*
*/ internal UI_Size ui_top_pref_width(void) { return ui_state->pref_width->v; } /*
*/ internal UI_Size ui_top_pref_height(void) { return ui_state->pref_height->v; } /*
*/ internal UI_Axis2 ui_top_child_layout_axis(void) { return ui_state->child_layout_axis->v; } /*

*/ internal v4 ui_top_background_color(void) { return ui_state->background_color->v; } /*
*/ internal v4 ui_top_text_color(void) { return ui_state->text_color->v; } /*
*/ internal v4 ui_top_border_color(void) { return ui_state->border_color->v; } /*

*/ internal FP_Handle ui_top_font_handle(void) { return ui_state->font_handle->v; } /*
*/ internal f32 ui_top_font_size(void) { return ui_state->font_size->v; } /*

*/ internal f32 ui_top_corner_radius(void) { return ui_state->corner_radius->v; } /*
*/ internal f32 ui_top_border_thickness(void) { return ui_state->border_thickness->v; } /*
*/
////////////////////////////////////////////////////////////////////////////////
