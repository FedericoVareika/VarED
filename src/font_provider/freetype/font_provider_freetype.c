
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Helpers

internal FP_FontHandle fp_ft_font_handle_from_face(FT_Face *face) {
    FP_FontHandle result = {
        .v = (u64)face,
    }; 
    return result;
}

internal FT_Face *fp_ft_face_from_font_handle(FP_FontHandle handle) {
    FT_Face *result = (FT_Face *)handle.v;
    return result;
}

////////////////////////////////////////////////////////////////////////////////

global FT_State *ft_state = 0;

internal void fp_init(void) {
    Arena *arena = arena_alloc();
    ft_state = push_struct(arena, FT_State);
    ft_state->arena = arena;

    FT_Error error = FT_Init_FreeType(&ft_state->ft_lib);
    assert(!error);
}

internal FP_FontHandle fp_open_font(char *filepath) {
    // TODO(fede): Use FT_New_Memory_Face and handle the file memory ourselves
    FT_Face *face = push_struct(ft_state->arena, FT_Face);

    FT_Error error = FT_New_Face(ft_state->ft_lib, filepath, 0, face);
    assert(!error);

    error = FT_Select_Charmap(*face, FT_ENCODING_UNICODE);
    assert(!error);

    return fp_ft_font_handle_from_face(face);
}

internal FP_FontMetrics fp_get_font_metrics(FP_FontHandle font, f32 size) {
    FT_Face face = *fp_ft_face_from_font_handle(font);
    FT_Error error = FT_Set_Pixel_Sizes(face, 0, (u32)((96.0f / 72.0f) * size));
    assert(!error);

    return (FP_FontMetrics){
        .ascender = (f32)(face->size->metrics.ascender >> 6),
        .descender = (f32)(face->size->metrics.descender >> 6),
        .height = (f32)(face->size->metrics.height >> 6),
    };
}

internal FP_GlyphMetrics fp_get_character_metrics(FP_FontHandle font, u32 codepoint, f32 size) {
    FT_Face face = *fp_ft_face_from_font_handle(font);
    FT_GlyphSlot slot = face->glyph;

    FT_Error error = FT_Set_Pixel_Sizes(face, 0, (u32)((96.0f / 72.0f) * size));
    assert(!error);

    error = FT_Load_Char(face, codepoint, FT_LOAD_BITMAP_METRICS_ONLY);
    assert(!error);

    FT_Glyph_Metrics metrics = slot->metrics;
    FP_GlyphMetrics result = {
        .bearing_x    = (f32)(metrics.horiBearingX >> 6),
        .bearing_y    = (f32)(metrics.horiBearingY >> 6),
        .width        = (f32)(metrics.width >> 6),
        .height       = (f32)(metrics.height >> 6),
        .advance      = (f32)(metrics.horiAdvance >> 6),
    };

    return result;
}

internal Bitmap2d fp_raster_character(Arena *arena, FP_FontHandle font, u32 codepoint, f32 size) {
    FT_Face face = *fp_ft_face_from_font_handle(font);

    FT_Error error = FT_Set_Pixel_Sizes(face, 0, (u32)((96.0f / 72.0f) * size));
    assert(!error);

    // STUDY(fede): Colored bitmaps (emojis)
    FT_GlyphSlot slot = face->glyph;
    error = FT_Load_Char(face, codepoint, FT_LOAD_RENDER);
    assert(!error);

    FT_Bitmap src = slot->bitmap;
    u32 left = slot->bitmap_left;
    u32 top = slot->bitmap_top;

    Bitmap2d result = {0};
    result.buf = push_array(arena, u8, src.rows * src.width * 4);
    result.width = src.width * 4;
    result.height = src.rows;
    result.stride = result.width * 4;

    for (u32 row = 0; row < src.rows; row++) {
        for (u32 x = 0; x < src.width; x++) {
            u32 offset = row * src.pitch + x;
            result.buf[(offset * 4)+0] = 255;
            result.buf[(offset * 4)+1] = 255;
            result.buf[(offset * 4)+2] = 255;
            result.buf[(offset * 4)+3] = src.buffer[offset];
        }
    }
        
    return result;
}
