#ifndef VARED_RENDERER_H
#define VARED_RENDERER_H

#include "vared_platform.h"
#include "vared_math.h"
#include "vared_arena.h"

#define MAX_VERTICES 3

typedef struct {
    v3 position; 
    v4 color;
    v2 uv;
} Vertex;

typedef struct {
    u32 vao;
    u32 vbo;
    u32 ebo;
    u32 shader_program;

    u32 font_texture;

    Vertex *vertex_buffer;
    u32 vertex_count;

    u16 *index_buffer;
    u32 index_count;
} Renderer;

typedef enum {
    RenderEntryType_None,

    // TODO(fede): Maybe the IR should not be rendering per se, more like 'Commands'
    RenderEntryType_TextureLoad,
    RenderEntryType_Quad,

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
    Arena push_buffer_arena;    // NOTE(fede): Base allocated on permanent storage 
    Arena vertex_buffer_arena;  // NOTE(fede): Shares base with Renderer vertex_buffer
    Arena index_buffer_arena;   // NOTE(fede): Shares base with Renderer index_buffer

    u32 width;
    u32 height;
} RenderGroup;

#endif // VARED_RENDERER_H
