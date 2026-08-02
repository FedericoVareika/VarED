
global FC_State *fc_state = 0;

internal void fc_init(void) {
    Arena *arena = arena_alloc();
    fc_state = push_struct(arena, FC_State);
    fc_state->arena = arena; 
    fc_state->frame_arena = arena_alloc();

    fc_state->scratch_raster_dst_size = kilobytes(2);
    fc_state->scratch_raster_dst = push_size(arena, fc_state->scratch_raster_dst_size);
}

internal void fc_tick(void) {
    arena_clear(fc_state->frame_arena);
}

internal FC_Glyph *fc_get_codepoint_glyph(FP_FontHandle font, u32 codepoint, f32 font_size) {

    FC_GlyphList *glyphs = &fc_state->glyphs;
    FC_GlyphNode *glyph_n = glyphs->first;
    for (u32 glyph_idx = 0; 
            glyph_idx < glyphs->count; 
            glyph_idx++, glyph_n = glyph_n->next) {
        if (glyph_n->v.codepoint == codepoint &&
                glyph_n->v.font_size == font_size) {
            break;
        }
    }

    if (!glyph_n) {
        glyph_n = push_struct(fc_state->arena, FC_GlyphNode);
        QueuePush(glyphs->first, glyphs->last, glyph_n);
        glyphs->count++;

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
            atlas_n = push_struct(fc_state->arena, FC_AtlasNode);
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
        atlas->next_y = max(atlas->next_y, atlas->first_free.y + pixel_height + 1);
        
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

    return &glyph_n->v;
}
