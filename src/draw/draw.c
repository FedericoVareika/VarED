
internal void dr_glyph_run(FP_FontMetrics metrics, FC_GlyphRun *glyph_run, v2 at, Rect2 clip, v4 color) {
    TimeFunction;

    v2 text_pos = at;
    text_pos.y += metrics.ascender;

    for (FC_GlyphPtrNode *glyph_ptr_n = glyph_run->first;
            glyph_ptr_n != 0;
            glyph_ptr_n = glyph_ptr_n->next) {

        FC_Glyph *glyph = glyph_ptr_n->v;

        if (glyph->codepoint != '\n' &&
                glyph->codepoint != '\t') {
            v2 pos = v2_add(text_pos, (v2){
                glyph->metrics.bearing_x,
                -glyph->metrics.bearing_y,
            });

            pos.x = round_f32_to_int(pos.x);
            pos.y = round_f32_to_int(pos.y);

            v2 dim = {
                glyph->metrics.width,
                glyph->metrics.height,
            };

            Rect2 glyph_rect = rect2_min_dim(pos, dim);

            if (glyph_rect.min.x > clip.max.x)
                break;

            // if (glyph_rect.min.y > clip.max.y)
            //     break;

            r_push_rect2(
                    .tex = glyph->tex,
                    .pos = glyph_rect,
                    .uv = glyph->uvs,
                    .clip = clip,
                    R_Color4(color));
        }

        text_pos.x += glyph->metrics.advance;
    }
}

internal void dr_text(FP_Handle font, f32 font_size, String8 text, v2 at, Rect2 clip, v4 color) {
    FC_GlyphRun *glyph_run = fc_get_string_glyph_run(font, text, font_size);

    FP_FontMetrics metrics = fp_get_font_metrics(font, font_size);

    dr_glyph_run(metrics, glyph_run, at, clip, color);
}
