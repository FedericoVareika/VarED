#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

#include <SDL2/SDL.h>
#include <GL/glew.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

typedef struct R_OpenGL_Tex2D R_OpenGL_Tex2D; 
struct R_OpenGL_Tex2D {
    R_OpenGL_Tex2D *next;

    GLuint id;
    u32 width;
    u32 height;
    GLint format;
};

typedef struct R_OpenGL_Tex2DList R_OpenGL_Tex2DList; 
struct R_OpenGL_Tex2DList {
    R_OpenGL_Tex2D *first;
    R_OpenGL_Tex2D *last;
    u32 count;
};

typedef struct R_OpenGL_Attrib R_OpenGL_Attrib; 
struct R_OpenGL_Attrib {
    GLint index;
    GLint size;
    GLenum type;
    GLsizei stride;
    u64 offset;
};

typedef struct R_OpenGL_AttribArray R_OpenGL_AttribArray;
struct R_OpenGL_AttribArray {
    R_OpenGL_Attrib *attributes;
    u32 count;
};

typedef struct R_OpenGL_BufferNode R_OpenGL_BufferNode;
struct R_OpenGL_BufferNode {
    R_OpenGL_BufferNode *next;
    GLuint buffer;
};

typedef struct R_OpenGL_BufferList R_OpenGL_BufferList;
struct R_OpenGL_BufferList {
    R_OpenGL_BufferNode *first;
    R_OpenGL_BufferNode *last;
};

typedef struct R_OpenGL_State R_OpenGL_State;
struct R_OpenGL_State {
    Arena *arena;
    GLuint vao;
    GLuint vbo;

    GLuint programs[R_ShaderType_Count];
    R_OpenGL_AttribArray program_attributes[R_ShaderType_Count];

    GLuint white_texture;
    R_OpenGL_Tex2DList textures;

    Arena *buffer_arena;
    R_OpenGL_BufferList buffers;
};

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Helpers

internal R_Handle r_ogl_handle_from_tex2d(R_OpenGL_Tex2D *tex);
internal R_OpenGL_Tex2D *r_ogl_tex2d_from_handle(R_Handle handle);

#endif // RENDER_OPENGL_H
