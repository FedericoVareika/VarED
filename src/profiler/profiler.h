#ifndef PROFILER_H
#define PROFILER_H

#ifndef PROFILER 
#define PROFILER 0
#endif // PROFILER

// STUDY(fede): Maybe i want this more lightweight.

typedef struct P_Key P_Key;
struct P_Key {
    u64 v;
};

typedef struct P_Anchor P_Anchor;
struct P_Anchor {
    P_Key key;
    String8 label;

    u64 inclusive_elapsed_time;
    u64 exclusive_elapsed_time;

    u64 hit_count;
    u64 processed_byte_count;
};

typedef struct P_AnchorNode P_AnchorNode;
struct P_AnchorNode {
    P_AnchorNode *parent;
    P_AnchorNode *next;
    P_AnchorNode *prev;
    P_AnchorNode *first;
    P_AnchorNode *last;

    P_Anchor v;
};

typedef struct P_FrameState P_FrameState;
struct P_FrameState {
    Arena *anchor_arena;
    // P_AnchorNode *root;
    // P_AnchorNode *parent;
    u32 parent_idx;

    // NOTE(fede): Assume indexing is exclusive for each node.
    //      If this isnt the case, we should implement a hash table of sorts,
    //      but it would be wasteful to do the key hashing in the profiler.
    u32 anchors_size;
    u32 last_anchor_idx;
    P_Anchor *anchors;

    u64 start_time;
    u64 end_time;
};

typedef struct P_State P_State;
struct P_State {
    Arena *arena;

    u32 frame_idx;
    P_FrameState frame_states[2];
};

typedef struct P_Block P_Block;
struct P_Block {
    String8 label;

    u32 anchor_idx;
    u32 parent_idx;
    // P_AnchorNode *anchor_n;

    u64 start_tsc;
    u64 old_inclusive_elapsed_time;
};

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): API

internal void p_init(void);
internal void p_begin(void);
internal void p_end(void);
internal void p_tick(void);
internal P_FrameState *p_previous_state(void);

#if PROFILER

// STUDY(fede): Is this portable? TODO
#define TimeBandwidth(anchor_label, byte_count)                                \
    P_Block __attribute__((unused)) __attribute__((                       \
        __cleanup__(p_block_destructor))) __profile_block##__LINE__ =      \
        p_construct_block(anchor_label, __COUNTER__ + 1, byte_count);
#define TimeBlock(anchor_label) TimeBandwidth(anchor_label, 0)
#define TimeFunction TimeBlock(S8(__func__))
#define TimeFunctionBandwidth(byte_count) TimeBandwidth(S8(__func__), byte_count)

#else

#define TimeBandwidth(...)
#define TimeBlock(...)
#define TimeFunction
#define TimeFunctionBandwidth(...)
#define ProfilerClear
#define ProfilerBegin
#define ProfilerEnd
#define ProfilerEndOfCompilationUnit

#endif // PROFILER == 1

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Helpers

// internal P_AnchorNode *p_anchor_from_key(P_Key key);
// internal inline bool p_anchor_is_nil(P_AnchorNode *anchor_n);

internal void p_block_destructor(P_Block *block);
internal P_Block p_construct_block(String8 label, u32 anchor_index, u64 byte_count);

////////////////////////////////////////////////////////////////////////////////

#endif // PROFILER_H

