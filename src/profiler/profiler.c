global P_State *p_state;
global P_FrameState *p_frame_state;

internal P_FrameState p_frame_init(void) {
    P_FrameState result = {0};
    result.anchor_arena = arena_alloc();

    return result;
}

internal void p_init(void) {
    Arena *arena = arena_alloc();
    p_state = push_struct(arena, P_State);
    p_state->arena = arena;

    p_state->frame_states[0] = p_frame_init();
    p_state->frame_states[1] = p_frame_init();
}

internal void p_begin(void) {
    p_frame_state->start_time = performance_counter();
}

internal void p_end(void) {
    p_frame_state->end_time = performance_counter();
}

internal void p_tick(void) {
    p_state->frame_idx = (p_state->frame_idx + 1) % 2;
    p_frame_state = &p_state->frame_states[p_state->frame_idx];

    arena_clear(p_frame_state->anchor_arena);
    p_frame_state->anchors_size = 4096;
    p_frame_state->anchors = push_array(
            p_frame_state->anchor_arena,
            P_Anchor,
            p_frame_state->anchors_size);
    mem_zero(p_frame_state->anchors, sizeof(P_Anchor) * p_frame_state->anchors_size);

    // p_frame_state->root = p_anchor_from_key((P_Key){1});
    // p_frame_state->parent = p_frame_state->root;
}

internal P_FrameState *p_previous_state(void) {
    return &p_state->frame_states[(p_state->frame_idx + 1) % 2];
}

internal void p_block_destructor(P_Block *block) {
    // P_AnchorNode *anchor_n = block->anchor_n;
    // P_AnchorNode *parent_n = anchor_n->parent;

    p_frame_state->parent_idx = block->parent_idx;

    u64 block_start_tsc = block->start_tsc;
    u64 old_inclusive_elapsed_time =
        block->old_inclusive_elapsed_time;

    u64 elapsed_time = performance_counter() - block_start_tsc;

    P_Anchor *anchor = &p_frame_state->anchors[block->anchor_idx];
    P_Anchor *parent = &p_frame_state->anchors[block->parent_idx];

    parent->exclusive_elapsed_time -= elapsed_time;
    anchor->exclusive_elapsed_time += elapsed_time;
    anchor->inclusive_elapsed_time = old_inclusive_elapsed_time + elapsed_time;

    anchor->label = block->label;
    anchor->hit_count++;
}

internal P_Block p_construct_block(String8 label, u32 anchor_idx, u64 byte_count) {
    // P_Key key = {anchor_index};
    // P_AnchorNode *anchor = p_anchor_from_key(key);
    P_Anchor *anchor = &p_frame_state->anchors[anchor_idx];
    P_Block block = {
        label,
        anchor_idx,
        p_frame_state->parent_idx,
        performance_counter(),
        anchor->inclusive_elapsed_time,
    };
    anchor->processed_byte_count += byte_count;
    p_frame_state->parent_idx = anchor_idx;

    // assert(!p_anchor_is_nil(p_frame_state->root));

    p_frame_state->last_anchor_idx = max(anchor_idx, p_frame_state->last_anchor_idx);

    return block;
}

/* TODO(fede): STUDY
 *
 *  How do i want to represent function calls to the same function that have 
 *  different parent trees? 
 *
 *  e.g.
 *      
 *      A
 *     / \
 *    B   C
 *   / \
 *  C   D
 *
 *  How do i represent C's time here? 
 *
 *  like this? (tree/flame graph)
 *  
 *  |--------A--------|
 *  |-----B-----|--C--|
 *  |--C--|--D--|
 */
/*
internal P_AnchorNode *p_anchor_from_key(P_Key key) {
    assert(key.v < p_frame_state->anchors_size);
    P_AnchorNode *result = &p_frame_state->anchors[key.v];

    if (p_anchor_is_nil(result)) {
        result = push_struct(p_frame_state->anchor_arena, P_AnchorNode);
        result->v.key = key;
    }

    result->first = result->last = result->next = result->prev = result->parent = &p_nil_anchor;
    result->parent = p_frame_state->parent;


    if (!p_anchor_is_nil(p_frame_state->parent)) {
        DLL_PushBack_nil(result->parent->first, result->parent->last, result, &p_nil_anchor);
    }

    return result;
}

internal inline bool p_anchor_is_nil(P_AnchorNode *anchor_n) {
    return !anchor_n || !anchor_n->v.key.v;
}
*/
