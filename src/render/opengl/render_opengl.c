
////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Copied code to check sdl errors

void scc(int code) {
    if (code < 0) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        assert(false);
    }
}

void *scp(void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
        assert(false);
    }
    return ptr;
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Log callback

internal const char *callback_type_as_cstr(GLenum type) {
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        return "ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "DEPRECATED_BEHAVIOR";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "UNDEFINED_BEHAVIOR";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "PERFORMANCE";
    case GL_DEBUG_TYPE_OTHER:
        return "OTHER";
    }
}

internal const char *callback_severity_as_cstr(GLenum severity) {
    switch (severity){
    case GL_DEBUG_SEVERITY_LOW:
        return "LOW";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "MEDIUM";
    case GL_DEBUG_SEVERITY_HIGH:
        return "HIGH";
    case GL_DEBUG_SEVERITY_NOTIFICATION: 
        return "NOTIFICATION";
    }
}

internal void MessageCallback(GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam)
{
    (void) source;
    (void) id;
    (void) length;
    (void) userParam;

    fprintf(stderr, "GL CALLBACK: type = ** %s **, severity = ** %s **, message = %s\n",
            callback_type_as_cstr(type),
            callback_severity_as_cstr(severity),
            message);
}

////////////////////////////////////////////////////////////////////////////////

global R_OpenGL_State *r_ogl_state = 0;

internal R_ShaderType r_shader_from_pass_type[R_PassType_Count] = {
    [R_PassType_None] = R_ShaderType_None,
    [R_PassType_UI] = R_ShaderType_UI,
};

char *r_vert_shader_filepaths[R_ShaderType_Count] = {
    [R_ShaderType_None] = 0,
    [R_ShaderType_UI] ="shaders/simple.vert",
};

// TODO(fede): static assert that the amount of shader types has not changed
char *r_frag_shader_filepaths[R_ShaderType_Count] = {
    [R_ShaderType_None] = 0,
    [R_ShaderType_UI] = "shaders/simple_text.frag",
};

GLint r_ogl_texture_format[R_TextureFormat_Count] = {
    [R_TextureFormat_R] = GL_RED,
    [R_TextureFormat_RGBA] = GL_RGBA,
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
    assert(shader_file.size);
    bool result = compile_shader_source(shader_file.memory, shader_type, shader);
    debug_platform_free_file_memory(0, shader_file);
    return result;
}

void r_platform_init(void) {
    Arena *arena = arena_alloc();
    r_ogl_state = push_struct(arena, R_OpenGL_State);
    r_ogl_state->arena = arena;

    {
        GLenum glewErr = glewInit();
        if (glewErr != GLEW_OK) {
            fprintf(stderr, "ERROR: Could not initialize GLEW: %s\n", glewGetErrorString(glewErr));
            return;
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (GLEW_ARB_debug_output) {
            glEnable(GL_DEBUG_OUTPUT);
            glDebugMessageCallback(MessageCallback, 0);
        } else {
            fprintf(stderr, "WARNING: GLEW_ARB_debug_output is not available");
        }

        scc(SDL_GL_SetSwapInterval(0));
    }

    glGenVertexArrays(1, &r_ogl_state->vao);
    glBindVertexArray(r_ogl_state->vao);

    glGenBuffers(1, &r_ogl_state->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, r_ogl_state->vbo);  
    glBufferData(GL_ARRAY_BUFFER, kilobytes(64), 0, GL_DYNAMIC_DRAW);

    {

        for (R_ShaderType i = R_ShaderType_None + 1; i < R_ShaderType_Count; i++) {
            GLuint vert_shader;
            char *vert_filepath = r_vert_shader_filepaths[i];
            assert(compile_shader_file(vert_filepath, GL_VERTEX_SHADER, &vert_shader));

            GLuint frag_shader;
            char *frag_filepath = r_frag_shader_filepaths[i];
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

            glDeleteShader(vert_shader);
            glDeleteShader(frag_shader);

            r_ogl_state->programs[i] = program;
        }
    }

    {
        glGenTextures(1, &r_ogl_state->white_texture);
        glBindTexture(GL_TEXTURE_2D, r_ogl_state->white_texture);
        u32 white_pixel = 0xFFFFFFFF;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white_pixel);
    }

    // NOTE(fede): UI Attributes
    {
#define X_VERTEX_UI_ATTRIBUTES \
    X(0, pos_rect, 4, FLOAT) \
    X(1, uv_rect, 4, FLOAT) \
    X(2, color, 4, FLOAT)

#define X(_, __, ___, ____) +1
        u32 n_vertex_attributes = 0 
            X_VERTEX_UI_ATTRIBUTES;
#undef X

        R_OpenGL_AttribArray *ui_attributes = &r_ogl_state->program_attributes[R_ShaderType_UI];
        ui_attributes->attributes = push_array(r_ogl_state->arena, R_OpenGL_Attrib, n_vertex_attributes);
        ui_attributes->count = n_vertex_attributes;

#define X(i, field, n_elems, elem_type) \
        ui_attributes->attributes[i] = (R_OpenGL_Attrib){ \
            .index=i, \
            .size=n_elems, \
            .type=GL_##elem_type, \
            .stride=sizeof(R_Rect2DInst), \
            .offset=offsetof(R_Rect2DInst, field), \
        };

        X_VERTEX_UI_ATTRIBUTES
#undef X
    }
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Helpers

internal R_Handle r_ogl_handle_from_tex2d(R_OpenGL_Tex2D *tex) {
    R_Handle result = {
        .v = (u64)tex,
    }; 
    return result;
}

internal R_OpenGL_Tex2D *r_ogl_tex2d_from_handle(R_Handle handle) {
    R_OpenGL_Tex2D *result = (R_OpenGL_Tex2D *)handle.v;

    return result;
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Consume 

internal void r_consume_pass(R_Pass *pass) {
    R_BatchGroupList *batch_groups = &pass->batch_groups;
    R_BatchGroupNode *batch_group_n = batch_groups->first;

    for (u32 i = 0;
            i < batch_groups->count;
            i++, batch_group_n = batch_group_n->next) {
        R_BatchGroup *batch_group = &batch_group_n->v;

        if (batch_group->texture_handle.v == 0) {
            glBindTexture(GL_TEXTURE_2D, r_ogl_state->white_texture);
        } else {
            R_OpenGL_Tex2D *tex2d = r_ogl_tex2d_from_handle(batch_group->texture_handle);
            glBindTexture(GL_TEXTURE_2D, tex2d->id);
        }

        R_BatchList *batches = &batch_group->batches;

        // TODO(fede): Maybe we should allocate a new buffer if the default one 
        //      does not fit the batch, or separate the batch in multiple calls.
        assert(batches->byte_count < kilobytes(64));
        glBindBuffer(GL_ARRAY_BUFFER, r_ogl_state->vbo);

        GLintptr offset = 0;
        R_BatchNode *batch_n = batches->first;
        for (u32 j = 0;
                j < batches->batch_count;
                j++, batch_n = batch_n->next) {
            R_Batch *batch = &batch_n->v;

            glBufferSubData(GL_ARRAY_BUFFER, offset, batch->byte_count, batch->v);

            offset += batch->byte_count; 
        }
        assert(!batch_n);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 
                batches->byte_count / batches->bytes_per_inst);
    }

    assert(!batch_group_n);
}

internal void r_consume_passes(R_PassList *passes) {
    R_PassNode *pass_n = passes->first;
    for (u32 i = 0;
            i < passes->count;
            i++, pass_n = pass_n->next) {
        R_Pass *pass = &pass_n->v;
        switch (pass->type) {
        case R_PassType_UI: {
            R_ShaderType program = r_shader_from_pass_type[pass->type];
            GLuint ogl_program = r_ogl_state->programs[program];
            glUseProgram(ogl_program);

            GLuint location = glGetUniformLocation(ogl_program, "screen_resolution");
            glUniform2f(location, r_state->window_width, r_state->window_height);

            R_OpenGL_AttribArray program_attrib = r_ogl_state->program_attributes[program];

            for (u32 attrib_idx = 0; 
                    attrib_idx < program_attrib.count; 
                    attrib_idx++){

                R_OpenGL_Attrib attrib = program_attrib.attributes[attrib_idx];
                glEnableVertexAttribArray(attrib.index);
                glVertexAttribPointer( 
                        attrib.index, attrib.size, attrib.type,
                        GL_FALSE, attrib.stride, (GLvoid *)attrib.offset);
                glVertexAttribDivisor(attrib.index, 1);
            }

            r_consume_pass(pass);
        } break;

        case R_PassType_None:
        case R_PassType_Count:
            assert(!"Invalid pass type");
            break;
        }
    }

    assert(!pass_n);
}

// NOTE(fede): Consume hook 
internal void r_consume_all(void) {
    glClearColor(0x0, 0x0, 0x0, 0xFF);
    glClear(GL_COLOR_BUFFER_BIT);

    r_consume_passes(&r_state->passes);

    r_state->passes = (R_PassList){0};
    arena_clear(r_state->frame_arena);
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Other Hooks 

internal R_Handle r_alloc_tex2d(R_TextureFormat texture_format, u8 *buf, u32 width, u32 height, R_TextureFormat pixel_format) {
    R_OpenGL_Tex2D *tex2d = push_struct(r_ogl_state->arena, R_OpenGL_Tex2D);
    tex2d->width = width;
    tex2d->height = height;
    tex2d->format = r_ogl_texture_format[texture_format];

    GLint ogl_pixel_format = r_ogl_texture_format[pixel_format];

    R_OpenGL_Tex2DList *textures = &r_ogl_state->textures;
    QueuePush(textures->first, textures->last, tex2d);

    glGenTextures(1, &tex2d->id);
    glBindTexture(GL_TEXTURE_2D, tex2d->id);
    glTexImage2D(GL_TEXTURE_2D, 0, tex2d->format, width, height, 0, ogl_pixel_format, GL_UNSIGNED_BYTE, buf);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return r_ogl_handle_from_tex2d(tex2d);
}

internal void r_update_tex2d(R_Handle tex, Rect2 dst, u8 *src, R_TextureFormat format) {
    R_OpenGL_Tex2D *tex2d = r_ogl_tex2d_from_handle(tex);

    GLint ogl_format = r_ogl_texture_format[format];

    glBindTexture(GL_TEXTURE_2D, tex2d->id);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
            dst.min.x, dst.min.y,
            dst.max.x - dst.min.x, dst.max.y - dst.min.y,
            ogl_format, GL_UNSIGNED_BYTE, src);
}

////////////////////////////////////////////////////////////////////////////////
