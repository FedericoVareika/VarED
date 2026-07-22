#include "vared_renderer.h"
#include "vared_platform.h"

internal const char *shader_type_as_cstr(GLuint shader) {
    switch (shader) {
    case GL_VERTEX_SHADER:
        return "GL_VERTEX_SHADER";
    case GL_FRAGMENT_SHADER:
        return "GL_FRAGMENT_SHADER";
    }
}

internal bool compile_shader_source(const GLchar *shader_source, GLuint shader_type, GLuint *shader) {
    *shader = glCreateShader(shader_type);
    glShaderSource(*shader, 1, &shader_source, NULL);
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);
        fprintf(stderr, "ERROR: could not compile %s\n", shader_type_as_cstr(GL_VERTEX_SHADER));
        fprintf(stderr, "%.*s\n", message_size, message);
        return false;
    }

    return true;
}

internal void renderer_init(Renderer *renderer) {
    assert(renderer->vertex_buffer);
    assert(renderer->index_buffer);

    {
        glGenVertexArrays(1, &renderer->vao);
        glBindVertexArray(renderer->vao);

        glGenBuffers(1, &renderer->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);  
        glBufferData(GL_ARRAY_BUFFER,
                     renderer->vertex_count * sizeof(Vertex),
                     renderer->vertex_buffer,
                     GL_DYNAMIC_DRAW);

        // TODO(fede): should i do an ebo? performance maybe? 
        // glGenBuffers(1, &renderer->ebo);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ebo);  
        // glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        //              renderer->index_count * sizeof(u16),
        //              renderer->index_buffer,
        //              GL_DYNAMIC_DRAW);

        // NOTE(fede): vec3 position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
                0,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                (GLvoid *)offsetof(Vertex, position));

        // NOTE(fede): vec4 color
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
                1,
                4,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                (GLvoid *)offsetof(Vertex, color));
    }

    {
        GLuint vert_shader, frag_shader;

        {
            DebugReadFileResult vert_shader_file = debug_platform_read_entire_file(0, "shaders/simple.vert"); 
            DebugReadFileResult frag_shader_file = debug_platform_read_entire_file(0, "shaders/simple.frag"); 

            assert(compile_shader_source(vert_shader_file.memory, GL_VERTEX_SHADER, &vert_shader))
            assert(compile_shader_source(frag_shader_file.memory, GL_FRAGMENT_SHADER, &frag_shader))

            debug_platform_free_file_memory(0, vert_shader_file);
            debug_platform_free_file_memory(0, frag_shader_file);
        }

        renderer->shader_program = glCreateProgram();
        glAttachShader(renderer->shader_program, vert_shader);
        glAttachShader(renderer->shader_program, frag_shader);
        glLinkProgram(renderer->shader_program);

        GLint linked = 0;
        glGetProgramiv(renderer->shader_program, GL_LINK_STATUS, &linked);
        if (!linked) {
            GLchar message[1024];
            GLsizei message_size = 0;
            glGetProgramInfoLog(renderer->shader_program, sizeof(message), &message_size, message);
            fprintf(stderr, "ERROR: could not link shader program\n");
            fprintf(stderr, "%.*s\n", message_size, message);
            assert(false);
        }

        glUseProgram(renderer->shader_program);

        glDeleteShader(vert_shader);
        glDeleteShader(frag_shader);
    }
}

internal void renderer_sync(Renderer *renderer) {
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    renderer->vertex_count * sizeof(Vertex),
                    renderer->vertex_buffer);
}

internal void renderer_draw(Renderer *renderer, RenderGroup rg) {
    u32 buffer_idx = 0; 
    while (buffer_idx < rg.push_buffer_arena.used) {
        RenderEntryHeader *header = (RenderEntryHeader *)(rg.push_buffer_base + buffer_idx);
        buffer_idx += sizeof(RenderEntryHeader);

        switch (header->type) {
        case RenderEntryType_Quad: {
            RenderEntryQuad *quad = (RenderEntryQuad *)(rg.push_buffer_base + buffer_idx);
            buffer_idx += sizeof(RenderEntryQuad);

            glDrawElementsBaseVertex(
                    GL_TRIANGLES,
                    6,
                    GL_UNSIGNED_SHORT,
                    renderer->index_buffer + quad->index_offset,
                    quad->vertex_offset);
        } break;
        case RenderEntryType_Count:
        case RenderEntryType_None:
            assert(false);
        }
    }
}
