#ifndef DRAW_H
#define DRAW_H

internal void dr_glyph_run(FP_FontMetrics metrics, FC_GlyphRun *glyph_run, v2 at, Rect2 clip, v4 color);
internal void dr_text(FP_Handle font, f32 font_size, String8 text, v2 at, Rect2 clip, v4 color);

#endif // DRAW_H
