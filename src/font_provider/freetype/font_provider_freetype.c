
global FT_State *ft_state = 0;

internal void fp_init(void) {
    Arena *arena = arena_alloc();
    ft_state = push_struct(arena, FT_State);
    ft_state->arena = arena;

    FT_Error error = FT_Init_FreeType(&ft_state->ft_lib);
    assert(!error);
}

internal void fp_open_font(char *filepath) {
    // TODO(fede): Use FT_New_Memory_Face and handle the file memory ourselves
    FT_Error error = FT_New_Face(ft_state->ft_lib, filepath, 0, &ft_state->face);
    assert(!error);
}
