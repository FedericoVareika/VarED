#ifndef FONT_CACHE_H
#define FONT_CACHE_H

typedef struct {
    f32 bearing_x;
    f32 bearing_y;

    f32 width;
    f32 height;

    f32 advance;
} FontGlyph;

typedef struct {
    Bitmap2d atlas;
} FontCache;

#endif // FONT_CACHE_H
