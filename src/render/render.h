#ifndef RENDER_H
#define RENDER_H

typedef enum {
    R_ShaderType_None,

    R_ShaderType_UI,

    R_ShaderType_Count,
} R_ShaderType;

// TODO(fede): 
//      - Border
//      - White texture override (so that we dont have to change batch group
//          when drawing both rectangles and text and such).
typedef struct {
    v4 pos_rect;    // fmt: min | max 
    v4 uv_rect;     // fmt: min | max
    v4 color0;   
    v4 color1;   
    v4 color2;   
    v4 color3;   

    f32 corner_radius;
    f32 edge_softness;
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
    v4 color0;
    v4 color1;
    v4 color2;
    v4 color3;
    float corner_radius;
    float edge_softness;
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
internal void r_push_rect2_(R_Rect2Params params);
#define r_push_rect2(...) r_push_rect2_((R_Rect2Params){.tex = nil_texture, .color0 = WHITE_V4, .color1 = WHITE_V4, .color2 = WHITE_V4, .color3 = WHITE_V4, __VA_ARGS__})

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Platform dependent hooks

internal void r_platform_init(void);
internal void r_consume_all(void);
internal void r_end_frame(void);

internal R_Handle r_alloc_tex2d(R_TextureFormat texture_format, u8 *buf, u32 width, u32 height, R_TextureFormat pixel_format);
internal void r_update_tex2d(R_Handle tex, Rect2 dst, u8 *src, R_TextureFormat format);

#endif // RENDER_H
