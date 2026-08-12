#ifndef RENDER_H
#define RENDER_H

typedef enum {
    R_ShaderType_None,

    R_ShaderType_UI,

    R_ShaderType_Count,
} R_ShaderType;

typedef struct {
    v4 pos_rect;
    v4 uv_rect; 
    v4 clip_rect;
    v4 color0;   
    v4 color1;   
    v4 color2;   
    v4 color3;   

    f32 corner_radius;
    f32 edge_softness;
    f32 border_thickness;
    f32 ignore_texture;
} R_Rect2DInst;

typedef enum {
    R_TextureFormat_R,
    R_TextureFormat_RGBA,
    R_TextureFormat_Count,
} R_TextureFormat; 

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Rendering pipeline (Structured in passes)

typedef struct {
    u64 v;
} R_Handle;

// TODO(fede): static assert size of VertexUI and such is smaller than this
#define BATCH_SIZE kilobytes(32)

typedef struct R_Batch R_Batch;
struct R_Batch {
    void *v;

    u64 byte_count;
    u64 byte_size;
};

typedef struct R_BatchNode R_BatchNode;
struct R_BatchNode {
    R_BatchNode *next;
    R_Batch v;
};

typedef struct R_BatchList R_BatchList;
struct R_BatchList {
    R_BatchNode *first;
    R_BatchNode *last;

    u64 byte_count;
    u64 bytes_per_inst;

    u32 batch_count;
};

typedef struct R_BatchGroup R_BatchGroup; 
struct R_BatchGroup {
    R_BatchList batches;

    R_Handle texture_handle;
    // Rect2 clip; 
};

typedef struct R_BatchGroupNode R_BatchGroupNode;
struct R_BatchGroupNode {
    R_BatchGroupNode *next;
    R_BatchGroup v;
};

typedef struct R_BatchGroupList R_BatchGroupList;
struct R_BatchGroupList {
    R_BatchGroupNode *first;
    R_BatchGroupNode *last;

    u32 count;
};

typedef enum {
    R_PassType_None,
    R_PassType_UI,
    R_PassType_Count,
} R_PassType;

typedef struct R_Pass R_Pass;
struct R_Pass {
    R_BatchGroupList batch_groups;    
    R_PassType type;                // STUDY(fede): Defines the shader as well
};

typedef struct R_PassNode R_PassNode;
struct R_PassNode {
    R_PassNode *next;
    R_Pass v;
};

typedef struct R_PassList R_PassList;
struct R_PassList {
    R_PassNode *first;
    R_PassNode *last;

    u32 count;
};

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Enums, Params and stuff

typedef struct R_Rect2Params R_Rect2Params;
struct R_Rect2Params {
    R_Handle tex;
    Rect2 pos;
    Rect2 uv;
    Rect2 clip;
    v4 color0;
    v4 color1;
    v4 color2;
    v4 color3;
    f32 corner_radius;
    f32 edge_softness;
    f32 border_thickness;
    f32 ignore_texture;
};

global const R_Handle nil_texture = {0};

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Render state

typedef struct R_State R_State;
struct R_State {
    Arena *arena;
    Arena *frame_arena;

    R_PassList passes;
    u32 window_width, window_height;
};

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Hooks

#define WHITE_V4 (v4){1, 1, 1, 1}
#define BLACK_V4 (v4){1, 1, 1, 1}

internal void r_init(u32 window_width, u32 window_height);
internal R_Rect2DInst *r_push_rect2_(R_Rect2Params params);
#define r_push_rect2(...) r_push_rect2_((R_Rect2Params){.tex = nil_texture, .color0 = WHITE_V4, .color1 = WHITE_V4, .color2 = WHITE_V4, .color3 = WHITE_V4, __VA_ARGS__})

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Platform dependent hooks

internal void r_platform_init(void);
internal void r_consume_all(void);
internal void r_end_frame(void);

internal R_Handle r_alloc_tex2d(R_TextureFormat texture_format, u8 *buf, u32 width, u32 height, R_TextureFormat pixel_format);
internal void r_update_tex2d(R_Handle tex, Rect2 dst, u8 *src, R_TextureFormat format);

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Helper Macros

#define R_Color4(c) .color0 = (c), .color1 = (c), .color2 = (c), .color3 = (c)

#endif // RENDER_H
