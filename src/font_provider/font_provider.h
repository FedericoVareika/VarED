#ifndef FONT_PROVIDER_H
#define FONT_PROVIDER_H

typedef struct FP_GlyphMetrics FP_GlyphMetrics; 
struct FP_GlyphMetrics {
    f32 bearing_x;
    f32 bearing_y;

    f32 width;
    f32 height;

    f32 advance;
};

internal void fp_init(void);
internal void fp_open_font(char *filepath);

internal FP_GlyphMetrics fp_get_character_metrics(u32 codepoint, f32 size);
internal Bitmap2d fp_raster_character(Arena *arena, u32 codepoint, f32 size);

#endif // FONT_PROVIDER_H
