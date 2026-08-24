
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
    R_PassList *passes = &r_state->top_bucket->v->passes;
    R_PassNode *pass_n = passes->last;
    if (!pass_n || pass_n->v.type != R_PassType_UI) {
        pass_n = push_struct(r_state->frame_arena, R_PassNode);
        pass_n->v.type = R_PassType_UI;
        QueuePush(passes->first, passes->last, pass_n);
        passes->count++;
    } 

    return pass_n;
} 

internal R_BatchGroupNode *r_get_batch_group_n(
        R_Pass *pass,
        R_Handle texture_handle,
        u64 inst_size) {
    R_BatchGroupNode *group_n = pass->batch_groups.last;

    if (!group_n || 
            group_n->v.batches.bytes_per_inst != inst_size ||
            (texture_handle.v != 0 && 
             group_n->v.texture_handle.v != 0 &&  
             group_n->v.texture_handle.v != texture_handle.v)) {
        group_n = push_struct(r_state->frame_arena, R_BatchGroupNode);
        group_n->v.texture_handle = texture_handle;
        group_n->v.batches.bytes_per_inst = inst_size;
        QueuePush(pass->batch_groups.first, pass->batch_groups.last, group_n);
        pass->batch_groups.count++;
    }

    return group_n;
}

#define r_push_batch_inst(batches, type) \
    (type *)r_push_batch_inst_(batches, sizeof(type))

internal void *r_push_batch_inst_(R_BatchList *batches, u64 inst_bytes) {
    R_BatchNode *batch_n = batches->last;
    if (!batch_n || batch_n->v.byte_size - batch_n->v.byte_count < inst_bytes) {
        batch_n = push_struct(r_state->frame_arena, R_BatchNode);
        batch_n->v.v = push_size(r_state->frame_arena, BATCH_SIZE);
        batch_n->v.byte_size = BATCH_SIZE;

        QueuePush(batches->first, batches->last, batch_n);
        // SLL_PushBack(batches->first, batches->last, batch_n);
        batches->batch_count++;
    }

    void *dst = (u8 *)batch_n->v.v + batch_n->v.byte_count;

    batch_n->v.byte_count += inst_bytes;
    batches->byte_count += inst_bytes;
    return dst;
}

internal R_Rect2DInst *r_push_rect2_(R_Rect2Params params) {
    R_PassNode *pass_n = r_get_pass_n(R_PassType_UI); 
    R_Pass *pass = &pass_n->v;

    R_BatchGroupNode *batch_group_n = r_get_batch_group_n(
            pass, params.tex, sizeof(R_Rect2DInst));

    R_BatchList *batches = &batch_group_n->v.batches;
    assert(batches->bytes_per_inst == sizeof(R_Rect2DInst));

    R_Rect2DInst *rect_inst = r_push_batch_inst(batches, R_Rect2DInst);
    rect_inst->pos_rect = params.pos.V4;
    rect_inst->uv_rect = params.uv.V4,
    rect_inst->clip_rect = params.clip.V4,
    rect_inst->color0 = params.color0;
    rect_inst->color1 = params.color1;
    rect_inst->color2 = params.color2;
    rect_inst->color3 = params.color3;
    rect_inst->corner_radius = params.corner_radius;
    rect_inst->edge_softness = params.edge_softness;
    rect_inst->border_thickness = params.border_thickness;
    rect_inst->ignore_texture = (params.tex.v == nil_texture.v) ? 1 : 0;

    if (rect_inst->ignore_texture < 1 &&
            batch_group_n->v.texture_handle.v == nil_texture.v) {
        batch_group_n->v.texture_handle = params.tex;
    }

    return rect_inst;
}

internal R_Bucket *r_get_new_bucket() {
    return push_struct(r_state->frame_arena, R_Bucket);
}

internal void r_push_bucket(R_Bucket *bucket) {
    R_BucketNode *bucket_n = push_struct(r_state->frame_arena, R_BucketNode);
    bucket_n->v = bucket;
    bucket_n->next = r_state->top_bucket;
    r_state->top_bucket = bucket_n;
}

internal void r_pop_bucket() {
    r_state->top_bucket = r_state->top_bucket->next;
}

internal void r_feed_top_bucket(R_Bucket *bucket) {
    R_Bucket *top_bucket = r_state->top_bucket->v;

    R_PassList *passes = &bucket->passes;

    R_PassNode *pass_n = passes->first;
    for (u32 i = 0;
            i < passes->count;
            i++, pass_n = pass_n->next) {
        R_Pass *src = &pass_n->v;
        R_PassNode *dst_n = r_get_pass_n(src->type);
        R_Pass *dst = &dst_n->v;

        dst->batch_groups.last->next = src->batch_groups.first;
        dst->batch_groups.last = src->batch_groups.last;
        dst->batch_groups.count += src->batch_groups.count;
    }
}

