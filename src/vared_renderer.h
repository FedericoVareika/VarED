#ifndef VARED_RENDERER_H
#define VARED_RENDERER_H

#include "vared_platform.h"
#include "vared_math.h"
#include "vared_arena.h"

#define MAX_VERTICES 3

typedef struct {
    v3 position; 
    v4 color;
} Vertex;

typedef struct {
    u32 vao;
    u32 vbo;
    u32 ebo;
    u32 shader_program;

    Vertex *vertex_buffer;
    u32 vertex_count;

    u16 *index_buffer;
    u32 index_count;
} Renderer;

typedef enum {
    RenderEntryType_None,

    // RenderGroupEntryType_Clear,
    RenderEntryType_Quad,

    RenderEntryType_Count,
} RenderEntryType;

typedef struct {
    RenderEntryType type;
} RenderEntryHeader;

typedef struct {
    u32 vertex_offset;
    u32 index_offset;
} RenderEntryQuad;

typedef struct {
    Arena push_buffer_arena;
    u8 *push_buffer_base;

    Arena vertex_buffer_arena;
    Vertex *vertex_buffer_; 

    Arena index_buffer_arena;
    u16 *index_buffer_; 

    u32 width;
    u32 height;
} RenderGroup;

#endif // VARED_RENDERER_H
