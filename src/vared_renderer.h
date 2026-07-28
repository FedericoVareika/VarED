#ifndef VARED_RENDERER_H
#define VARED_RENDERER_H

#include "vared_platform.h"

#define MAX_VERTICES 3

typedef enum {
    ShaderType_None,

    ShaderType_Text,
    ShaderType_Color,

    ShaderType_Count,
} ShaderType;

typedef struct {
    v3 position; 
    v4 color;
    v2 uv;
} Vertex;

typedef enum {
    RenderEntryType_None,

    // TODO(fede): Maybe the IR should not be rendering per se, more like 'Commands'
    RenderEntryType_TextureLoad,
    RenderEntryType_Quad,
    RenderEntryType_ShaderSet,

    RenderEntryType_Count,
} RenderEntryType;

typedef struct {
    RenderEntryType type;
} RenderEntryHeader;

// TODO(fede): Change to batch rendering. ** IMPORTANT **
typedef struct {
    Vertex vertices[4];
    u16 indices[6];
} RenderEntryQuad;

typedef struct {
    u8 *data;
    u32 width, height;
} RenderEntryTextureLoad;

typedef struct {
    ShaderType shader;
} RenderEntryShaderSet;

typedef struct {
    RenderEntryType type;
    union {
        RenderEntryQuad quad;
        RenderEntryTextureLoad texture_load;
        RenderEntryShaderSet shader_set;
    };
} RenderEntry;

typedef struct RenderEntryNode RenderEntryNode;
struct RenderEntryNode {
    RenderEntryNode *next;
    RenderEntry v;
};

typedef struct {
    RenderEntryNode *first;
    RenderEntryNode *last;
    u32 count;
} RenderEntryList;

typedef struct {
    RenderEntryList entries;
    u32 width;
    u32 height;
} RenderGroup;

#endif // VARED_RENDERER_H
