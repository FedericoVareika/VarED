#ifndef VARED_RENDERER_OPENGL_H 
#define VARED_RENDERER_OPENGL_H 

#include "vared_renderer.h"

typedef struct {
    u32 vao;
    u32 vbo;
    u32 ebo;

    u32 programs[ShaderType_Count];
    ShaderType current_shader;

    u32 font_texture;

    Vertex *vertex_buffer;
    u32 vertex_count;

    u16 *index_buffer;
    u32 index_count;
} RendererOpengl;

#endif // VARED_RENDERER_OPENGL_H 
