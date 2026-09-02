#ifndef THREAD_CONTEXT_H
#define THREAD_CONTEXT_H

typedef struct T_Context T_Context;
struct T_Context {
    u32 arena_count;
    Arena *arenas[2];
};

internal void t_context_init(void);

internal Temp scratch_begin(Arena **conflicts, u32 n);
#define scratch_end(scratch) temp_end(scratch)

#endif // THREAD_CONTEXT_H
