#ifndef FONT_CACHE_H
#define FONT_CACHE_H

typedef struct FC_Glyph FC_Glyph;
struct FC_Glyph {
    f32 bearing_x;
    f32 bearing_y;

    f32 width;
    f32 height;

    f32 advance;

    Rect2 uvs;
};

typedef struct FC_GlyphNode FC_GlyphNode;
struct FC_GlyphNode {
    FC_GlyphNode *next;
    FC_Glyph v;
};

typedef struct FC_GlyphList FC_GlyphList;
struct FC_GlyphList {
    FC_GlyphNode *first;
    FC_GlyphNode *last;

    u32 count;
};

typedef struct FC_State FC_State;
struct FC_State {
    // TODO(fede): Change to hash map
    // FC_GlyphList glyphs;

    Arena *arena;
    Arena *frame_arena;
};

internal void fc_init(void);

internal FontGlyph *fc_get_codepoint_glyph(u32 codepoint);

#endif // FONT_CACHE_H
