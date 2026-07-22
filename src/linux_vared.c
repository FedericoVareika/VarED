#define SDL_INCLUDE_STDBOOL_H 0
#include <SDL2/SDL.h>
#include <GL/glew.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#include "vared.h"
#include "vared_renderer.c"

#include "linux_vared.h"
#include "vared_renderer_opengl.c"

#include <sys/mman.h>

#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include <dlfcn.h>

#include <errno.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

global bool global_editor_running = true;

#if VARED_INTERNAL

internal DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file) {
    DebugReadFileResult result = {0};

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        // handle error
        return (DebugReadFileResult){0};
    }

    struct stat stat_;
    if (stat(filename, &stat_) == -1) {
        // handle error
        close(fd);
        return (DebugReadFileResult){0};
    }

    assert(sizeof(stat_.st_size) == sizeof(u64));
    result.size = stat_.st_size;

    result.memory = mmap(0, result.size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    u64 bytes_read = read(fd, result.memory, result.size);
    if (bytes_read != result.size) {
        // handle error
        close(fd);
        return (DebugReadFileResult){0};
    }

    close(fd);

    return result;
}

internal DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory) {
    if (file_result.memory) {
        munmap(file_result.memory, file_result.size);
    }
}

internal DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file) {
    /*
     * NOTE(fede): When O_CREAT flag is set, a *mode* flag must be set as well.
     *
     *    In this case:
     *
     *         S_IRWXU -- 00700 user (file owner) has read, write, and
     *                    execute permission
     *
     */

    int fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);
    if (fd == -1) {
        // handle error
        return false;
    }

    u64 bytes_written = write(fd, memory, size);
    if (bytes_written != size) {
        // handle error
        return false;
    }

    close(fd);

    return true;
}

#endif

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

internal int string_len(char *str) {
    int result = 0;
    while (*str++)
        result++;
    return result;
}

internal void cat_strings(int source_a_count, char *source_a,
                          int source_b_count, char *source_b, int dest_count,
                          char *dest) {
    for (int i = 0; i < source_a_count; i++) {
        *dest++ = *source_a++;
    }

    for (int i = 0; i < source_b_count; i++) {
        *dest++ = *source_b++;
    }

    *dest++ = 0;
}

internal void linux_get_exe_path(LinuxState *state) {
    int filename_len = readlink("/proc/self/exe", state->exe_filename,
                                array_count(state->exe_filename));

    state->one_past_last_slash = state->exe_filename + filename_len;
    for (char *scan = state->exe_filename; *scan; scan++) {
        if (*scan == '/') {
            state->one_past_last_slash = scan + 1;
        }
    }
}

internal void linux_build_global_filename_at_exe_location(LinuxState *state,
                                                          char *dest,
                                                          char *filename) {
    cat_strings(state->one_past_last_slash - state->exe_filename,
                state->exe_filename, string_len(filename), filename,
                LINUX_FILEPATH_MAX_COUNT, dest);
}

EDITOR_UPDATE_AND_RENDER(editor_update_and_render_stub) {}

internal bool linux_editor_has_changed(EditorLib *editor, char *filename) {
    bool result = false;

    struct stat stat_;
    if (stat(filename, &stat_) == -1) {
        // TODO(fede): logging
        assert(!"File does not exist.");
    };

    if (stat_.st_size == 0) {
        return false;
    }

    if ((editor->last_modified.tv_sec != stat_.st_mtim.tv_sec) ||
        (editor->last_modified.tv_nsec != stat_.st_mtim.tv_nsec)) {
        result = true;
        editor->last_modified = stat_.st_mtim;
    }

    return result;
}

internal EditorLib linux_load_editorlib(char *filename) {
    EditorLib result = {0};

    result.handle = dlopen(filename, RTLD_NOW);

    if (result.handle) {
        result.is_valid = true;
        // NOTE(fede): iso c bullshit
        *(void**)(&result.update_and_render) = dlsym(result.handle, "editor_update_and_render");
    } else {
        result.is_valid = false;
        result.update_and_render = editor_update_and_render_stub;
    }

    return result;
}

internal void linux_reload_editorlib(EditorLib *editor, char *filename) {
    if (editor->is_valid)
        dlclose(editor->handle);
    editor->handle = dlopen(filename, RTLD_NOW);

    if (editor->handle) {
        editor->is_valid = true;
        // NOTE(fede): iso c bullshit
        *(void**)(&editor->update_and_render) = dlsym(editor->handle, "editor_update_and_render");
    } else {
        editor->is_valid = false;
        editor->update_and_render = editor_update_and_render_stub;
    }
}

internal void linux_unload_editorlib(EditorLib *editor) {
    if (editor->is_valid) {
        dlclose(editor->handle);
        editor->update_and_render = editor_update_and_render_stub;
        editor->is_valid = false;
    } else {
        assert(!editor->handle);
    }
}

int main(void) {
    LinuxState state = {0};

    linux_get_exe_path(&state);
    char editor_dll_filename[LINUX_FILEPATH_MAX_COUNT];

    linux_build_global_filename_at_exe_location(&state, editor_dll_filename,
                                                "vared.so");
    
#if VARED_INTERNAL
    void *base_address = (void *)terabytes((u64)2);
#else
    void *base_address = 0;
#endif

    EditorMemory editor_memory = {0};
    editor_memory.permanent_storage_size = megabytes(64);
    editor_memory.transient_storage_size = gigabytes((u64)1);
    {
        state.editor_memory_size = editor_memory.permanent_storage_size +
                                 editor_memory.transient_storage_size;
        state.editor_memory_block =
            mmap(base_address, state.editor_memory_size, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        editor_memory.permanent_storage = state.editor_memory_block;
        editor_memory.transient_storage =
            (u8 *)state.editor_memory_block + editor_memory.permanent_storage_size;

        editor_memory.debug_platform_read_entire_file =
            &debug_platform_read_entire_file;
        editor_memory.debug_platform_free_file_memory =
            &debug_platform_free_file_memory;
        editor_memory.debug_platform_write_entire_file =
            &debug_platform_write_entire_file;
    }

    // NOTE(fede): init sdl
    scc(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window *window = scp(SDL_CreateWindow(
                "VarED", 
                0, 0,
                WINDOW_WIDTH, WINDOW_HEIGHT,
                SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL));

    // NOTE(fede): init opengl
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        int major;
        int minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        printf("GL version %d.%d\n", major, minor);
    }

    SDL_GLContext gl_context = scp(SDL_GL_CreateContext(window));
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        fprintf(stderr, "ERROR: Could not initialize GLEW: %s\n", glewGetErrorString(glewErr));
        return 1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (GLEW_ARB_debug_output) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(MessageCallback, 0);
    } else {
        fprintf(stderr, "WARNING: GLEW_ARB_debug_output is not available");
    }

    EditorLib editor = {0};

    editor = linux_load_editorlib(editor_dll_filename);
    if (!editor.is_valid) {
        printf("Editor is not valid: %s\n", dlerror());
        printf("dll filename: %s\n", editor_dll_filename);
    }

    Renderer renderer;
    {
        u32 max_quads = 100 * 1000; // 100_000
        u32 vertex_count = max_quads * 4;
        u32 index_count = max_quads * 6;

        u32 vertex_buffer_size = vertex_count * sizeof(Vertex);
        u32 index_buffer_size = index_count * sizeof(i16);

        // TODO(fede): Check if this should use the base address as before 
        //          or include in previous mmap
        u8 *buffer_memory = mmap(0, vertex_buffer_size + index_buffer_size,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        renderer = (Renderer){
            .vertex_buffer = (Vertex *)buffer_memory,
            .vertex_count = vertex_count,
            .index_buffer = (u16 *)(buffer_memory + vertex_buffer_size),
            .index_count = index_count,
        };

        renderer_init(&renderer);
    }

    RenderGroup render_group = {0};
    render_group.width = WINDOW_WIDTH;
    render_group.height = WINDOW_HEIGHT;
    rg_init_vertex_index_buffers(&render_group, &renderer);

    while (global_editor_running) {
        if (linux_editor_has_changed(&editor, editor_dll_filename)) {
            linux_reload_editorlib(&editor, editor_dll_filename);
            if (!editor.is_valid) {
                printf("Editor is not valid: %s\n", dlerror());
            }
        }

        SDL_Event event = {0};

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                global_editor_running = false;
            } break;
            case SDL_WINDOWEVENT: {
                SDL_WindowEvent window_event = event.window;
                switch (window_event.event) {
                case SDL_WINDOWEVENT_RESIZED: {

                    // NOTE(fede): 
                    //  window_event.data1 -> new width
                    //  window_event.data2 -> new height
                    render_group.width = window_event.data1;
                    render_group.height = window_event.data2;
                } break;
                }
            } break;
            }
        }

        // TODO(fede): editor input
        editor.update_and_render(&editor_memory, 0, &render_group);

        glClearColor(0x0, 0x0, 0x0, 0xFF);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer_sync(&renderer);
        renderer_draw(&renderer, render_group);

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
