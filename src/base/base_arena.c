
// TODO(fede): alignment, arena chains, etc.
internal Arena *arena_alloc_(ArenaParams params) {
    assert(params.reserve_size % params.commit_size == 0);

    void *base = mem_reserve(params.reserve_size);
    assert(mem_commit(base, params.commit_size));
    mem_zero(base, params.commit_size);

    Arena *arena = (Arena *)base;
    *arena = (Arena) {
        .reserve_size = params.reserve_size,
        .commit_size = params.commit_size,
        .commited = params.commit_size,
        .base = base,
        .base_pos = 0,
        .pos = ARENA_HEADER_SIZE,
    };

    return arena;
}

internal u64 arena_pos(Arena *arena) {
    return arena->base_pos + arena->pos;
}

// TODO(fede): push aligned
internal void *push_size(Arena *arena, u64 size) {
    u64 new_pos = arena_pos(arena) + size;
    assert(new_pos <= arena->reserve_size);

    if (arena->commited < new_pos) {
        u64 new_commit_size = new_pos - arena->commited;
        u64 n_commits = (new_commit_size + arena->commit_size - 1) 
            / arena->commit_size;

        new_commit_size = n_commits * arena->commit_size;
        u8 *commit_pos = arena->base + arena->commited;
        mem_commit(commit_pos, new_commit_size);
        mem_zero(commit_pos, new_commit_size);

        arena->commited += new_commit_size;
    }

    assert(arena->commited >= new_pos);

    void *result = (u8 *)arena + arena_pos(arena);
    arena->pos += size;
    return result;
}

internal void arena_release(Arena *arena) {
    assert(mem_release(arena->base, arena->reserve_size));
    arena->reserve_size = 0;
    arena->commited = 0;
}

// TODO(fede): Change to arena_pop_to(arena, 0);
internal void arena_clear(Arena *arena) {
    assert(arena->commited >= arena->commit_size);
    assert(arena->commited % arena->commit_size == 0);

    u64 decommit_size = arena->commited - arena->commit_size;
    assert(mem_decommit(arena->base + arena->commit_size, decommit_size));
    arena->commited = arena->commit_size;
    arena->pos = ARENA_HEADER_SIZE;

    mem_zero(arena->base + arena_pos(arena), arena->commited - arena_pos(arena));
}
