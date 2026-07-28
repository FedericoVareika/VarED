#ifndef VARED_ARENA_H
#define VARED_ARENA_H

// NOTE(fede): Veery similar implementation to Ryan Fleury's raddbg arena. 
//      I use it as an example implementation, as well as an example of project 
//      structure (raddbg). 

#define ARENA_HEADER_SIZE 128

typedef struct {
    u64 reserve_size;
    u64 commit_size;
    u64 commited;
    
    u64 base_pos;
    u64 pos;
    u8 *base;
} Arena;

global u64 arena_default_reserve_size = megabytes(64);
global u64 arena_default_commit_size  = kilobytes(64);

typedef struct {
    u64 reserve_size;
    u64 commit_size;
} ArenaParams;

#define arena_alloc(...) arena_alloc_((ArenaParams){.reserve_size = arena_default_reserve_size, .commit_size = arena_default_commit_size, __VA_ARGS__})
internal Arena *arena_alloc_(ArenaParams params);

internal void arena_release(Arena *arena);
internal void arena_clear(Arena *arena);

/* STUDY(fede):
 *
 * Struct *s = arena_bootstrap_struct(arena, Struct)
 * s->arena ....
 *
 *
 * */


// TODO(fede): zero flag?

internal void *push_size(Arena *arena, u64 size); 

#define push_struct(arena, Type) (Type *)push_size((arena), sizeof(Type))
#define push_array(arena, Type, count) (Type *)push_size((arena), sizeof(Type) * (count))

#endif // VARED_ARENA_H

