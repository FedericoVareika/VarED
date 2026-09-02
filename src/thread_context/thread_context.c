
global T_Context *t_context = 0;

internal void t_context_init(void) {
    Arena *arena_0 = arena_alloc();
    Arena *arena_1 = arena_alloc();

    t_context = push_struct(arena_0, T_Context);
    t_context->arena_count = 2;
    t_context->arenas[0] = arena_0;
    t_context->arenas[1] = arena_1;
}

internal Temp scratch_begin(Arena **conflicts, u32 n) {
    Arena *available_arena = 0;

    for (u32 i = 0; i < t_context->arena_count; i++) {
        available_arena = t_context->arenas[i];
        bool is_available = true;
        for (u32 j = 0; j < n; j++) {
            if (conflicts[j] == available_arena) {
                is_available = false;
                break;
            }
        }

        if (is_available) {
            break;
        } else {
            available_arena = 0;
        }
    }

    assert(available_arena);
    Temp result = temp_begin(available_arena);
    return result;
}
