#ifndef FONT_CACHE_H
#define FONT_CACHE_H

typedef struct FC_Glyph FC_Glyph;
struct FC_Glyph {
    FP_GlyphMetrics metrics;

    R_Handle tex;
    Rect2 uvs;

    // TODO(fede): Font handle too?
    u32 codepoint;
    f32 font_size;
};

typedef struct FC_GlyphNode FC_GlyphNode;
struct FC_GlyphNode {
    FC_GlyphNode *next;
    FC_Glyph v;
};

typedef struct FC_GlyphHashSlot FC_GlyphHashSlot;
struct FC_GlyphHashSlot {
    FC_GlyphNode *hash_first;
    FC_GlyphNode *hash_last;
};

typedef struct FC_RunKey FC_RunKey;
struct FC_RunKey {
    u64 v;
};

typedef struct FC_GlyphRun FC_GlyphRun;
struct FC_GlyphRun {
    FC_GlyphNode *first;
    FC_GlyphNode *last;

    FC_RunKey key;
    u32 count;
    f32 advance;
};

typedef struct FC_GlyphRunNode FC_GlyphRunNode;
struct FC_GlyphRunNode {
    FC_GlyphRunNode *next;
    FC_GlyphRunNode *prev;
    FC_GlyphRun v;
};

typedef struct FC_GlyphRunHashSlot FC_GlyphRunHashSlot;
struct FC_GlyphRunHashSlot {
    FC_GlyphRunNode *hash_first;
    FC_GlyphRunNode *hash_last;
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
    Arena *arena;
    Arena *frame_arena;

    FC_GlyphHashSlot *glyph_table;
    u32 glyph_table_size;

    Arena *run_hash_arena;
    FC_GlyphRunHashSlot *run_table; 
    u32 run_table_size;

    FC_AtlasList atlases;

    void *scratch_raster_dst;
    u64 scratch_raster_dst_size;
};

internal void fc_init(void);
internal void fc_tick(void);

internal FC_Glyph *fc_get_codepoint_glyph(FP_FontHandle font, u32 codepoint, f32 font_size);
internal FC_GlyphRun *fc_get_string_glyph_run(FP_FontHandle font, String8 string, f32 font_size);

// TODO(fede): Font handle too?
internal FC_RunKey fc_run_key_from_string_size(String8 string, f32 font_size);
internal bool fc_run_key_match(FC_RunKey a, FC_RunKey b);


#endif // FONT_CACHE_H
