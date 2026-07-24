#include "vared_renderer.h"
#include "vared_renderer_opengl.h"
#include "vared_platform.h"

char *vert_shader_filepath = "shaders/simple.vert";

// TODO(fede): static assert that the amount of shader types has not changed
char *frag_shader_filepaths[ShaderType_Count] = {
    [ShaderType_None] = 0,
    [ShaderType_Color] = "shaders/simple_color.frag",
    [ShaderType_Text] = "shaders/simple_text.frag",
};

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

internal bool compile_shader_file(char *shader_filepath, GLuint shader_type, GLuint *shader) {
    DebugReadFileResult shader_file = debug_platform_read_entire_file(0, shader_filepath); 
    bool result = compile_shader_source(shader_file.memory, shader_type, shader);
    debug_platform_free_file_memory(0, shader_file);
    return result;
}

internal void renderer_init(RendererOpengl *renderer) {
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
        glGenBuffers(1, &renderer->ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ebo);  
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     renderer->index_count * sizeof(u16),
                     renderer->index_buffer,
                     GL_DYNAMIC_DRAW);

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

        // NOTE(fede): vec2 uv
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(
                2,
                2,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                (GLvoid *)offsetof(Vertex, uv));
    }

    {
        GLuint vert_shader;
        assert(compile_shader_file(vert_shader_filepath, GL_VERTEX_SHADER, &vert_shader));

        for (ShaderType i = ShaderType_None + 1; i < ShaderType_Count; i++) {
            GLuint frag_shader;
            char *frag_filepath = frag_shader_filepaths[i];
            assert(compile_shader_file(frag_filepath, GL_FRAGMENT_SHADER, &frag_shader));

            GLuint program = glCreateProgram();
            glAttachShader(program, vert_shader);
            glAttachShader(program, frag_shader);
            glLinkProgram(program);

            GLint linked = 0;
            glGetProgramiv(program, GL_LINK_STATUS, &linked);
            if (!linked) {
                GLchar message[1024];
                GLsizei message_size = 0;
                glGetProgramInfoLog(program, sizeof(message), &message_size, message);
                fprintf(stderr, "ERROR: could not link shader program\n");
                fprintf(stderr, "%.*s\n", message_size, message);
                assert(!"Could not link shader program");
            }
            glDeleteShader(frag_shader);

            renderer->programs[i] = program;
        }

        glDeleteShader(vert_shader);
    }
}

internal void renderer_sync(RendererOpengl *renderer) {
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    renderer->vertex_count * sizeof(Vertex),
                    renderer->vertex_buffer);

    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                    0,
                    renderer->index_count * sizeof(u16),
                    renderer->index_buffer);
}

internal void set_shader(RendererOpengl *renderer, RenderGroup rg, ShaderType shader) {
    renderer->current_shader = shader;
    glUseProgram(renderer->programs[renderer->current_shader]);

    // TODO(fede): change when we have multiple uniforms.
    GLuint location = glGetUniformLocation(
            renderer->programs[renderer->current_shader], "screen_resolution");
    glUniform2f(location, rg.width, rg.height);
}

#define consume_render_entry(type, var, buffer, idx) do { \
    var = (type *)(buffer + idx); \
    idx += sizeof(type); } while(0)


internal void renderer_draw(RendererOpengl *renderer, RenderGroup rg) {
    u8 *push_buffer = rg.push_buffer_arena.base;

    u32 buffer_idx = 0; 
    while (buffer_idx < rg.push_buffer_arena.used) {
        RenderEntryHeader *header = (RenderEntryHeader *)(push_buffer + buffer_idx);
        buffer_idx += sizeof(RenderEntryHeader);

        switch (header->type) {
        case RenderEntryType_TextureLoad: {
            RenderEntryTextureLoad *texture_load;
            consume_render_entry(RenderEntryTextureLoad, texture_load,
                    push_buffer, buffer_idx);

            // TODO(fede): Consume this into something else so that it is used 
            //      apart from fonts.
            glGenTextures(1, &renderer->font_texture);
            glBindTexture(GL_TEXTURE_2D, renderer->font_texture);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_RED,
                    texture_load->width, texture_load->height,
                    0, GL_RED, GL_UNSIGNED_BYTE, texture_load->data);

            // NOTE(fede): Do not know what this does. (linear filtering ?)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            glBindTexture(GL_TEXTURE_2D, renderer->font_texture);
        } break;
        case RenderEntryType_Quad: {
            RenderEntryQuad *quad;
            consume_render_entry(RenderEntryQuad, quad,
                    push_buffer, buffer_idx);

            glDrawRangeElementsBaseVertex(
                    GL_TRIANGLES,
                    quad->index_offset,
                    quad->index_offset + 6,
                    6,
                    GL_UNSIGNED_SHORT,
                    NULL,
                    quad->vertex_offset);
        } break;
        case RenderEntryType_ShaderSet: {
            RenderEntryShaderSet *shader_set;
            consume_render_entry(RenderEntryShaderSet, shader_set,
                    push_buffer, buffer_idx);

            set_shader(renderer, rg, shader_set->shader);
        } break;
        case RenderEntryType_Count:
        case RenderEntryType_None:
            assert(false);
        }
    }
}
