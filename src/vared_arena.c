#include "vared_arena.h"

internal void initialize_arena(
        Arena *arena,
        MemoryIndex size,
        u8 *base) {
    arena->size = size;
    arena->base = base;
    arena->used = 0;
} 

internal void *push_size(Arena *arena, MemoryIndex size) {
    assert(size <= arena->size - arena->used);
    void *result = arena->base + arena->used; 
    arena->used += size;
    return result;
}

#define push_struct(arena, Type) (Type *)push_size((arena), sizeof(Type))
#define push_array(arena, Type, count) (Type *)push_size((arena), sizeof(Type) * (count))

// #define arena_foreach(arena, var, Type) for (Type var = (arena)->base; var < (arena)->base + (arena)->size; var += sizeof(Type))
