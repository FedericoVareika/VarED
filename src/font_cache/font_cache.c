
global FC_State *fc_state = 0;

internal void fc_init(void) {
    Arena *arena = arena_alloc();
    fc_state = push_struct(arena, FC_State);
    fc_state->arena = arena; 
    fc_state->frame_arena = arena_alloc();
    fc_state->caching_arena = arena_alloc();

    fc_state->frame_idx = 0;

    fc_state->scratch_raster_dst_size = kilobytes(2);
    fc_state->scratch_raster_dst = push_size(arena, fc_state->scratch_raster_dst_size);

    fc_state->glyph_table_size = 100;
    fc_state->glyph_table = push_array(fc_state->caching_arena, FC_GlyphHashSlot, fc_state->glyph_table_size);

    fc_state->run_hash_arena = arena_alloc();
    fc_state->run_table_size = 100;
    fc_state->run_table = push_array(fc_state->run_hash_arena, FC_GlyphRunHashSlot, fc_state->run_table_size);
}

internal void fc_tick(void) {
    TimeFunction;

    arena_clear(fc_state->frame_arena);

    for (u32 i = 0; i < fc_state->glyph_table_size; i++) {
        FC_GlyphHashSlot *slot = &fc_state->glyph_table[i];

        for (FC_GlyphNode *glyph_n = slot->hash_first; glyph_n != 0;) {
            FC_GlyphNode *next_n = glyph_n->next;

            if (glyph_n->v.last_frame_touched_idx != fc_state->frame_idx) {
                DLL_Remove(slot->hash_first, slot->hash_last, glyph_n);
                glyph_n->next = fc_state->first_free_glyph;
                fc_state->first_free_glyph = glyph_n;
            }

            glyph_n = next_n;
        }
    }

    fc_state->frame_idx++;
}

internal void fc_flush(void) {
    arena_clear(fc_state->caching_arena);
    fc_state->glyph_table = push_array(fc_state->caching_arena, FC_GlyphHashSlot, fc_state->glyph_table_size);
    fc_state->first_free_glyph = 0;

    arena_clear(fc_state->run_hash_arena);
    fc_state->run_table = push_array(fc_state->run_hash_arena, FC_GlyphRunHashSlot, fc_state->run_table_size);
}

internal FC_Glyph *fc_get_codepoint_glyph(FP_Handle font, u32 codepoint, f32 font_size) {
    FC_GlyphHashSlot *slot = &fc_state->glyph_table[codepoint % fc_state->glyph_table_size];
    FC_GlyphNode *glyph_n = slot->hash_first;
    for (; glyph_n != 0; glyph_n = glyph_n->next) {

        if (glyph_n->v.codepoint == codepoint &&
                glyph_n->v.font_size == font_size) {
            break; 
        }
    }

    if (!glyph_n) {
        if (fc_state->first_free_glyph) {
            glyph_n = fc_state->first_free_glyph;
            fc_state->first_free_glyph = fc_state->first_free_glyph->next;
        } else {
            glyph_n = push_struct(fc_state->caching_arena, FC_GlyphNode);
        }

        DLL_PushBack(slot->hash_first, slot->hash_last, glyph_n);

        glyph_n->v.codepoint = codepoint;
        glyph_n->v.font_size = font_size;
        glyph_n->v.metrics = fp_get_character_metrics(font, codepoint, font_size);

        u32 pixel_width = ceil_f32_to_int(glyph_n->v.metrics.width);
        u32 pixel_height = ceil_f32_to_int(glyph_n->v.metrics.height);

        FC_AtlasList *atlases = &fc_state->atlases;
        FC_AtlasNode *atlas_n = 0;

        if (atlases->last) {
            FC_Atlas *atlas = &atlases->last->v;

            v2u space = v2u_sub(atlas->dim, atlas->first_free);
            if (space.x >= pixel_width && space.y >= pixel_height) {
                atlas_n = atlases->last;
            } else if (pixel_height <= atlas->dim.y - atlas->next_y) {
                atlas_n = atlases->last;

                atlas->first_free.x = 0;
                atlas->first_free.y = atlas->next_y;
            }
        }

        if (!atlas_n) {
            atlas_n = push_struct(fc_state->caching_arena, FC_AtlasNode);
            QueuePush(atlases->first, atlases->last, atlas_n);
            atlases->count++;

            FC_Atlas *atlas = &atlas_n->v;
            atlas->dim = (v2u){ 512, 512 };
            atlas->tex = r_alloc_tex2d(
                    R_TextureFormat_RGBA,
                    0, atlas->dim.x, atlas->dim.y,
                    R_TextureFormat_RGBA);
        }

        FC_Atlas *atlas = &atlas_n->v;

        v2 first_free_f = { atlas->first_free.x, atlas->first_free.y };
        Rect2 atlas_dst = rect2_min_dim(first_free_f, (v2){ pixel_width, pixel_height });

        atlas->first_free.x += pixel_width + 1;
        atlas->first_free.x = min(atlas->first_free.x, atlas->dim.x);
        atlas->next_y = max(atlas->next_y, atlas->first_free.y + pixel_height + 1);
        atlas->next_y = min(atlas->next_y, atlas->dim.y);
        
        Bitmap2d raster = fp_raster_character(
                fc_state->frame_arena,
                font,
                codepoint,
                font_size);

        r_update_tex2d(atlas->tex, atlas_dst, raster.buf, R_TextureFormat_RGBA);

        {
            glyph_n->v.tex = atlas->tex;

            glyph_n->v.uvs = atlas_dst;

            glyph_n->v.uvs.min.x /= (f32)atlas->dim.x;
            glyph_n->v.uvs.max.x /= (f32)atlas->dim.x;

            glyph_n->v.uvs.min.y /= (f32)atlas->dim.y;
            glyph_n->v.uvs.max.y /= (f32)atlas->dim.y;
        }
    }

    glyph_n->v.last_frame_touched_idx = fc_state->frame_idx;

    return &glyph_n->v;
}

internal FC_GlyphRun *fc_get_string_glyph_run(
        FP_Handle font,
        String8 string,
        f32 font_size) {

    FC_RunKey key = fc_run_key_from_string_size(font, string, font_size);
    FC_GlyphRunHashSlot *slot = &fc_state->run_table[key.v % fc_state->run_table_size];
    FC_GlyphRunNode *run_n = slot->hash_first;

    for (; run_n != 0; run_n = run_n->next) {
        if (fc_run_key_match(key, run_n->v.key) && run_n->v.font_size == font_size) {
            break;
        }
    }

    FC_GlyphRun *run;
    if (!run_n) {
        run_n = push_struct(fc_state->run_hash_arena, FC_GlyphRunNode);
        DLL_PushBack(slot->hash_first, slot->hash_last, run_n);

        run = &run_n->v;

        run->key = key;
        run->advance = 0;
        run->count = 0;
        run->font_size = font_size;
        run->last_frame_touched_idx = -1;
    }

    run = &run_n->v;
    
    // TODO(fede): This is hacky, means that we should redo the run if it wasnt 
    //      touched the previous frame, such that the glyph nodes would already 
    //      be cleaned up. I do not think this is appropiate, and it is a 
    //      workaround mostly for font increasing/decreasing.
    if (run->last_frame_touched_idx + 1 < fc_state->frame_idx) {
        u32 prev_glyph = 0;
        run->count = 0;
        run->advance = 0;

        FC_GlyphPtrNode *glyph_ptr_n = run->first;
        for (u32 i = 0; i < string.size; glyph_ptr_n = glyph_ptr_n->next) {
            if (glyph_ptr_n == 0) {
                glyph_ptr_n = push_struct(fc_state->run_hash_arena, FC_GlyphPtrNode);
                DLL_PushBack(run->first, run->last, glyph_ptr_n);
            }

            String8 substring = str8_skip(string, i);
            UnicodeCodepoint codepoint = utf8_decode(substring.str, substring.size);
            i += codepoint.byte_size;

            glyph_ptr_n->v = fc_get_codepoint_glyph(font, codepoint.character, font_size);

            run->advance += glyph_ptr_n->v->metrics.advance; 
            run->count++;

            prev_glyph = glyph_ptr_n->v->metrics.glyph_idx;
        }
    }

    for (FC_GlyphPtrNode *glyph_ptr_n = run_n->v.first;
            glyph_ptr_n != 0;
            glyph_ptr_n = glyph_ptr_n->next) {
        glyph_ptr_n->v->last_frame_touched_idx = fc_state->frame_idx;
    }

    run->last_frame_touched_idx = fc_state->frame_idx;

    return &run_n->v;
}

internal FC_RunKey fc_run_key_from_string_size(FP_Handle font, String8 string, f32 font_size) {
    FC_RunKey result = {0};

    u64 seed = font_size;
    result.v = str8_hash_u64_seed(string, seed);

    return result;
}

internal bool fc_run_key_match(FC_RunKey a, FC_RunKey b) {
    return a.v == b.v;
}
