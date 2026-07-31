
global FC_State *fc_state = 0;

internal void fc_init(void) {
    Arena *arena = arena_alloc();
    fc_state = push_struct(arena, FC_State);
    fc_state->arena = arena; 
    fc_state->frame_arena = arena_alloc();
}


internal FontGlyph *fc_get_codepoint_glyph(u32 codepoint) {

}
