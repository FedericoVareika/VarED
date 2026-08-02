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

typedef struct FP_FontHandle FP_FontHandle; 
struct FP_FontHandle {
    u64 v;
};

typedef struct FP_FontMetrics FP_FontMetrics;
struct FP_FontMetrics {
    f32 ascender;
    f32 descender;
    f32 height;
};

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Platform dependent hooks

internal void fp_init(void);
internal FP_FontHandle fp_open_font(char *filepath);

internal FP_FontMetrics fp_get_font_metrics(FP_FontHandle font, f32 size);

internal FP_GlyphMetrics fp_get_character_metrics(FP_FontHandle font, u32 codepoint, f32 size);
internal Bitmap2d fp_raster_character(Arena *arena, FP_FontHandle font, u32 codepoint, f32 size);

#endif // FONT_PROVIDER_H
