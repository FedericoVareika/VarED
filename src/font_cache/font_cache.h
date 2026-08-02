#ifndef FONT_CACHE_H
#define FONT_CACHE_H

typedef struct FC_Glyph FC_Glyph;
struct FC_Glyph {
    FP_GlyphMetrics metrics;

    R_Handle tex;
    Rect2 uvs;

    u32 codepoint;
    f32 font_size;
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

typedef struct FC_Atlas FC_Atlas;
struct FC_Atlas {
    R_Handle tex;

    v2u dim; 
    v2u first_free; 

    u32 next_y;
};

typedef struct FC_AtlasNode FC_AtlasNode;
struct FC_AtlasNode {
    FC_AtlasNode *next;
    FC_Atlas v;
};

typedef struct FC_AtlasList FC_AtlasList;
struct FC_AtlasList {
    FC_AtlasNode *first;
    FC_AtlasNode *last;
    u32 count;
};

typedef struct FC_State FC_State;
struct FC_State {
    // TODO(fede): Change to hash map
    FC_GlyphList glyphs;

    FC_AtlasList atlases;

    Arena *arena;
    Arena *frame_arena;

    void *scratch_raster_dst;
    u64 scratch_raster_dst_size;
};

internal void fc_init(void);
internal void fc_tick(void);

internal FC_Glyph *fc_get_codepoint_glyph(FP_FontHandle font, u32 codepoint, f32 font_size);

#endif // FONT_CACHE_H
