#ifndef VARED_RENDERER_H
#define VARED_RENDERER_H

#include "vared_platform.h"
#include "vared_math.h"
#include "vared_arena.h"

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
    u32 vertex_offset;
    u32 index_offset;
} RenderEntryQuad;

typedef struct {
    u8 *data;
    u32 width, height;
} RenderEntryTextureLoad;

typedef struct {
    ShaderType shader;
} RenderEntryShaderSet;

typedef struct {
    Arena push_buffer_arena;    // NOTE(fede): Base allocated on permanent storage 
    Arena vertex_buffer_arena;  // NOTE(fede): Shares base with Renderer vertex_buffer
    Arena index_buffer_arena;   // NOTE(fede): Shares base with Renderer index_buffer

    u32 width;
    u32 height;
} RenderGroup;

#endif // VARED_RENDERER_H
