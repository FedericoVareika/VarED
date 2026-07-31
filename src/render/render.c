
global const R_Handle nil_texture = {0};

global R_State *r_state = 0;

internal void r_init(u32 window_width, u32 window_height) {
    Arena *arena = arena_alloc();
    r_state = push_struct(arena, R_State);
    r_state->arena = arena;
    r_state->frame_arena = arena_alloc();

    r_state->window_width = window_width;
    r_state->window_height = window_width;
}

internal R_PassNode *r_get_pass_n(R_PassType type) {
    R_PassNode *pass_n = r_state->passes.last;
    if (!pass_n || pass_n->v.type != R_PassType_UI) {
        pass_n = push_struct(r_state->frame_arena, R_PassNode);
        pass_n->v.type = R_PassType_UI;
        QueuePush(r_state->passes.first, r_state->passes.last, pass_n);
        r_state->passes.count++;
    } 

    return pass_n;
} 

internal R_BatchGroupNode *r_get_batch_group_n(
        R_Pass *pass,
        R_Handle texture_handle,
        u64 inst_size) {
    R_BatchGroupNode *group_n = pass->batch_groups.last;

    if (!group_n 
            || group_n->v.texture_handle.v != texture_handle.v
            || group_n->v.batches.bytes_per_inst != inst_size) {
        group_n = push_struct(r_state->frame_arena, R_BatchGroupNode);
        group_n->v.texture_handle = texture_handle;
        group_n->v.batches.bytes_per_inst = inst_size;
        QueuePush(pass->batch_groups.first, pass->batch_groups.last, group_n);
        pass->batch_groups.count++;
    }

    return group_n;
}

internal void r_push_batch_inst(R_BatchList *batches, void *v, u64 inst_bytes) {
    R_BatchNode *batch_n = batches->last;
    if (!batch_n || batch_n->v.byte_size - batch_n->v.byte_count < inst_bytes) {
        batch_n = push_struct(r_state->frame_arena, R_BatchNode);
        batch_n->v.v = push_size(r_state->frame_arena, BATCH_SIZE);
        batch_n->v.byte_size = BATCH_SIZE;

        QueuePush(batches->first, batches->last, batch_n);
        batches->batch_count++;
    }

    void *dst = (u8 *)batch_n->v.v + batch_n->v.byte_count;
    mem_copy(dst, v, inst_bytes);

    batch_n->v.byte_count += inst_bytes;
    batches->byte_count += inst_bytes;
}

internal void r_push_rect2(R_Handle tex, Rect2 pos, Rect2 uv, v4 color) {
    R_PassNode *pass_n = r_get_pass_n(R_PassType_UI); 
    R_Pass *pass = &pass_n->v;

    R_BatchGroupNode *batch_group_n = r_get_batch_group_n(pass, tex, sizeof(R_Rect2DInst));
    R_BatchList *batches = &batch_group_n->v.batches;
    assert(batches->bytes_per_inst == sizeof(R_Rect2DInst));

    v2 pos_dim = rect2_dim(pos);
    
    R_Rect2DInst *rect_inst = push_struct(r_state->frame_arena, R_Rect2DInst);
    rect_inst->pos_rect = (v4){ .xy = pos.min, .zw = pos_dim };
    rect_inst->uv_rect = uv.V4,
    rect_inst->color = color;

    void *v = (void *)rect_inst;
    u64 byte_count = sizeof(R_Rect2DInst);

    r_push_batch_inst(batches, v, byte_count);
}
