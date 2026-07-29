#define SDL_INCLUDE_STDBOOL_H 0
#include <SDL2/SDL.h>
#include <GL/glew.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#include "base/base_inc.h"
#include "base/base_inc.c"

#include "vared_platform.h"
#include "linux_vared.h"

#include "vared_renderer_opengl.c"
#include "vared_renderer.c"

#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

global bool global_editor_running = true;
global u64 performance_frequency;

internal inline f64 sdl_get_seconds_elapsed(u64 start_counter,
                                            u64 end_counter) {
    return (f64)(end_counter - start_counter) / performance_frequency;
}

internal void linux_sleep_to_target(u64 last_counter, f64 target_seconds) {
    f64 seconds_elapsed_for_frame;

    struct timespec sleep_time = {0};
    struct timespec remaining_time = {0};
    do {
        seconds_elapsed_for_frame =
            sdl_get_seconds_elapsed(last_counter, SDL_GetPerformanceCounter());

        // NOTE(fede): truncate the amount of ms to sleep
        f64 ms_to_sleep =
            (f64)(u64)(1000.0f * (target_seconds - seconds_elapsed_for_frame));

        // NOTE(fede): give 1,000,000 ns of leeway
        ms_to_sleep -= 1.0f;

        u64 nsec_to_sleep = (u64)(ms_to_sleep * 1000000.0f);

        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = nsec_to_sleep;
    } while (nanosleep(&sleep_time, &remaining_time) == -1);

    seconds_elapsed_for_frame =
        sdl_get_seconds_elapsed(last_counter, SDL_GetPerformanceCounter());

    if (seconds_elapsed_for_frame <= target_seconds) {
        while (seconds_elapsed_for_frame < target_seconds)
            seconds_elapsed_for_frame = sdl_get_seconds_elapsed(
                last_counter, SDL_GetPerformanceCounter());
    } else {
        // TODO(fede): ERROR -- missed target frame rate
        printf("Missed target frame rate!\n");
    }
}

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

internal WMEvent *wm_event_list_add(Arena *arena, WMEventList *event_list) {
    
}

internal WMKey sdl_get_wm_key(SDL_KeyCode key_code) {
    switch (key_code) {
    case SDLK_UNKNOWN: 
        return WMKey_NONE;

    case SDLK_RETURN: 
        return WMKey_RETURN;
    case SDLK_ESCAPE: 
        return WMKey_ESCAPE;
    case SDLK_BACKSPACE: 
        return WMKey_BACKSPACE;
    case SDLK_TAB: 
        return WMKey_TAB;
    case SDLK_SPACE: 
        return WMKey_SPACE;
    case SDLK_EXCLAIM: 
        return WMKey_EXCLAIM;
    case SDLK_QUOTEDBL: 
        return WMKey_QUOTEDBL;
    case SDLK_HASH: 
        return WMKey_HASH;
    case SDLK_PERCENT: 
        return WMKey_PERCENT;
    case SDLK_DOLLAR: 
        return WMKey_DOLLAR;
    case SDLK_AMPERSAND: 
        return WMKey_AMPERSAND;
    case SDLK_QUOTE: 
        return WMKey_QUOTE;
    case SDLK_LEFTPAREN: 
        return WMKey_LEFTPAREN;
    case SDLK_RIGHTPAREN: 
        return WMKey_RIGHTPAREN;
    case SDLK_ASTERISK: 
        return WMKey_ASTERISK;
    case SDLK_PLUS: 
        return WMKey_PLUS;
    case SDLK_COMMA: 
        return WMKey_COMMA;
    case SDLK_MINUS: 
        return WMKey_MINUS;
    case SDLK_PERIOD: 
        return WMKey_PERIOD;
    case SDLK_SLASH: 
        return WMKey_SLASH;
    case SDLK_0: 
        return WMKey_0;
    case SDLK_1: 
        return WMKey_1;
    case SDLK_2: 
        return WMKey_2;
    case SDLK_3: 
        return WMKey_3;
    case SDLK_4: 
        return WMKey_4;
    case SDLK_5: 
        return WMKey_5;
    case SDLK_6: 
        return WMKey_6;
    case SDLK_7: 
        return WMKey_7;
    case SDLK_8: 
        return WMKey_8;
    case SDLK_9: 
        return WMKey_9;
    case SDLK_COLON: 
        return WMKey_COLON;
    case SDLK_SEMICOLON: 
        return WMKey_SEMICOLON;
    case SDLK_LESS: 
        return WMKey_LESS;
    case SDLK_EQUALS: 
        return WMKey_EQUALS;
    case SDLK_GREATER: 
        return WMKey_GREATER;
    case SDLK_QUESTION: 
        return WMKey_QUESTION;
    case SDLK_AT: 
        return WMKey_AT;

    case SDLK_LEFTBRACKET: 
        return WMKey_LEFTBRACKET;
    case SDLK_BACKSLASH: 
        return WMKey_BACKSLASH;
    case SDLK_RIGHTBRACKET: 
        return WMKey_RIGHTBRACKET;
    case SDLK_CARET: 
        return WMKey_CARET;
    case SDLK_UNDERSCORE: 
        return WMKey_UNDERSCORE;
    case SDLK_BACKQUOTE: 
        return WMKey_BACKQUOTE;
    case SDLK_a: 
        return WMKey_a;
    case SDLK_b: 
        return WMKey_b;
    case SDLK_c: 
        return WMKey_c;
    case SDLK_d: 
        return WMKey_d;
    case SDLK_e: 
        return WMKey_e;
    case SDLK_f: 
        return WMKey_f;
    case SDLK_g: 
        return WMKey_g;
    case SDLK_h: 
        return WMKey_h;
    case SDLK_i: 
        return WMKey_i;
    case SDLK_j: 
        return WMKey_j;
    case SDLK_k: 
        return WMKey_k;
    case SDLK_l: 
        return WMKey_l;
    case SDLK_m: 
        return WMKey_m;
    case SDLK_n: 
        return WMKey_n;
    case SDLK_o: 
        return WMKey_o;
    case SDLK_p: 
        return WMKey_p;
    case SDLK_q: 
        return WMKey_q;
    case SDLK_r: 
        return WMKey_r;
    case SDLK_s: 
        return WMKey_s;
    case SDLK_t: 
        return WMKey_t;
    case SDLK_u: 
        return WMKey_u;
    case SDLK_v: 
        return WMKey_v;
    case SDLK_w: 
        return WMKey_w;
    case SDLK_x: 
        return WMKey_x;
    case SDLK_y: 
        return WMKey_y;
    case SDLK_z: 
        return WMKey_z;

    case SDLK_CAPSLOCK: 
        return WMKey_CAPSLOCK;

    case SDLK_F1: 
        return WMKey_F1;
    case SDLK_F2: 
        return WMKey_F2;
    case SDLK_F3: 
        return WMKey_F3;
    case SDLK_F4: 
        return WMKey_F4;
    case SDLK_F5: 
        return WMKey_F5;
    case SDLK_F6: 
        return WMKey_F6;
    case SDLK_F7: 
        return WMKey_F7;
    case SDLK_F8: 
        return WMKey_F8;
    case SDLK_F9: 
        return WMKey_F9;
    case SDLK_F10: 
        return WMKey_F10;
    case SDLK_F11: 
        return WMKey_F11;
    case SDLK_F12: 
        return WMKey_F12;

    case SDLK_RIGHT: 
        return WMKey_RIGHT;
    case SDLK_LEFT: 
        return WMKey_LEFT;
    case SDLK_DOWN: 
        return WMKey_DOWN;
    case SDLK_UP: 
        return WMKey_UP;

    default:
        return 0;
    }
} 

internal WMModifiers sdl_get_wm_modifiers(SDL_Keymod mod) {
    if( mod == KMOD_NONE )
    return 0;

    WMModifiers result = 0;

    if( mod & KMOD_NUM )    result |= 0;
    if( mod & KMOD_CAPS )   result |= WMModifier_caps;
    if( mod & KMOD_LCTRL )  result |= 0;
    if( mod & KMOD_RCTRL )  result |= 0;
    if( mod & KMOD_RSHIFT ) result |= 0;
    if( mod & KMOD_LSHIFT ) result |= 0;
    if( mod & KMOD_RALT )   result |= 0;
    if( mod & KMOD_LALT )   result |= 0;
    if( mod & KMOD_CTRL )   result |= WMModifier_ctrl;
    if( mod & KMOD_SHIFT )  result |= WMModifier_shift;
    if( mod & KMOD_ALT )    result |= WMModifier_alt;

    return result;
} 

/* Print modifier info */
void PrintModifiers( SDL_Keymod mod ){
    printf( "Modifers: " );

    /* If there are none then say so and return */
    if ( mod == KMOD_NONE ){
        printf( "None\n" );
        return;
    }

    /* Check for the presence of each SDLMod value */
    /* This looks messy, but there really isn't    */
    /* a clearer way.                              */
    if( mod & KMOD_NUM ) printf( "NUMLOCK " );
    if( mod & KMOD_CAPS ) printf( "CAPSLOCK " );
    if( mod & KMOD_LCTRL ) printf( "LCTRL " );
    if( mod & KMOD_RCTRL ) printf( "RCTRL " );
    if( mod & KMOD_RSHIFT ) printf( "RSHIFT " );
    if( mod & KMOD_LSHIFT ) printf( "LSHIFT " );
    if( mod & KMOD_RALT ) printf( "RALT " );
    if( mod & KMOD_LALT ) printf( "LALT " );
    if( mod & KMOD_CTRL ) printf( "CTRL " );
    if( mod & KMOD_SHIFT ) printf( "SHIFT " );
    if( mod & KMOD_ALT ) printf( "ALT " );
    printf( "\n" );
}


void PrintKeyInfo( SDL_KeyboardEvent *key ){
    /* Is it a release or a press? */
    if( key->type == SDL_KEYUP )
        printf( "Release:- " );
    else
        printf( "Press:- " );

    /* Print the hardware scancode first */
    printf( "Scancode: 0x%02X", key->keysym.scancode );
    /* Print the name of the key */
    printf( ", Name: %s", SDL_GetKeyName( key->keysym.sym ) );
    /* We want to print the unicode info, but we need to make */
    /* sure its a press event first (remember, release events */
    /* don't have unicode info                                */
    if( key->type == SDL_KEYDOWN ){
    /* If the Unicode value is less than 0x80 then the    */
    /* unicode value can be used to get a printable       */
    /* representation of the key, using (char)unicode.    */
    // printf(", Unicode: " );
    // if( key->keysym.unicode < 0x80 && key->keysym.unicode > 0 ){
    //     printf( "%c (0x%04X)", (char)key->keysym.unicode,
    //             key->keysym.unicode );
    // }
    // else{
    //     printf( "? (0x%04X)", key->keysym.unicode );
    // }
    }
    printf( "\n" );
    /* Print modifier info */
    PrintModifiers( (SDL_Keymod)key->keysym.mod );
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

    void *memory = 0;
    EditorParams editor_params = {0};
    editor_params.memory = &memory;

    PlatformApi platform = {0};
    {
        platform.debug_platform_read_entire_file =
            &debug_platform_read_entire_file;
        platform.debug_platform_free_file_memory =
            &debug_platform_free_file_memory;
        platform.debug_platform_write_entire_file =
            &debug_platform_write_entire_file;
    }
    editor_params.platform = platform;

    Arena *event_arena = arena_alloc();
    Arena *renderer_arena = arena_alloc();

    // NOTE(fede): init sdl
    scc(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window *window = scp(SDL_CreateWindow(
            "VarED", 
            0, 0,
            WINDOW_WIDTH, WINDOW_HEIGHT,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL));

    // NOTE(fede): get highest display refresh rate
    int display_index = SDL_GetWindowDisplayIndex(window);
    int n_display_modes = SDL_GetNumDisplayModes(display_index);

    SDL_DisplayMode mode = {0};
    SDL_GetDisplayMode(display_index, 0, &mode); // highest

    performance_frequency = SDL_GetPerformanceFrequency();

    /*
    * STUDY(fede): This is 144hz for my machine and probably a lot more.
    *              However, we are doing software rendering, so ~30hz is
    *              probably the goal.
    *
    *              ** Investigate ways to chose FPS reliably. **
    */

    int refresh_rate = mode.refresh_rate;

    int game_update_rate = refresh_rate;
    f32 target_seconds_per_frame = 1.0f / (f32)game_update_rate;
    printf("refresh_rate: %d\n", refresh_rate);

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

    scc(SDL_GL_SetSwapInterval(0));

    EditorLib editor = {0};

    editor = linux_load_editorlib(editor_dll_filename);
    if (!editor.is_valid) {
        printf("Editor is not valid: %s\n", dlerror());
        printf("dll filename: %s\n", editor_dll_filename);
    }

    RendererOpengl renderer;
    {
        u32 max_quads = 100 * 1000; // 100_000
        u32 vertex_count = max_quads * 4;
        u32 index_count = max_quads * 6;

        Vertex *vertex_buffer = push_array(renderer_arena, Vertex, vertex_count); 
        u16 *index_buffer = push_array(renderer_arena, u16, index_count); 

        renderer = (RendererOpengl){
            .vertex_buffer = vertex_buffer,
            .vertex_count = vertex_count,
            .index_buffer = index_buffer,
            .index_count = index_count,
        };

        renderer_init(&renderer);
    }

    RenderGroup render_group = {0};
    render_group.width = WINDOW_WIDTH;
    render_group.height = WINDOW_HEIGHT;

    SDL_StartTextInput();

    u64 last_counter = SDL_GetPerformanceCounter();

    while (global_editor_running) {
    if (linux_editor_has_changed(&editor, editor_dll_filename)) {
        linux_reload_editorlib(&editor, editor_dll_filename);
        if (!editor.is_valid) {
            printf("Editor is not valid: %s\n", dlerror());
        }
    }

    WMEventList *event_list = push_struct(event_arena, WMEventList);
    SDL_Event event = {0};

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT: {
            global_editor_running = false;
        } break;
        case SDL_WINDOWEVENT: {
            SDL_WindowEvent window_event = event.window;
            switch (window_event.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED: {
                int render_width, render_height;
                SDL_GL_GetDrawableSize(window, &render_width, &render_height);

                glViewport(0, 0, render_width, render_height);

                render_group.width = render_width;
                render_group.height = render_height;
            } break;
            }
        } break;
        // case SDL_KEYUP:
        case SDL_KEYDOWN: {
            SDL_KeyboardEvent key_event = event.key;
            PrintKeyInfo(&key_event);
            SDL_Keymod mod = key_event.keysym.mod;

            // TODO(fede): push-back macro
            WMEventNode *event_n = push_struct(event_arena, WMEventNode);
            QueuePush(event_list->first, event_list->last, event_n);
            event_list->count++;

            WMEvent *event = &event_n->v;

            // TODO(fede): handle cursor
            
            event->key = sdl_get_wm_key(key_event.keysym.sym);
            event->modifiers = sdl_get_wm_modifiers(mod);
            event->repeat = key_event.repeat;
        } break;
        case SDL_TEXTINPUT: {
            u8 *text = (u8 *)event.text.text;
            u64 max_size = SDL_TEXTINPUTEVENT_TEXT_SIZE;
            // String8 text_str = S("ñ");
            // u8 *text = text_str.str;
            // u64 max_size = text_str.size;

            while (*text) {
                UnicodeCodepoint codepoint = utf8_decode(
                        text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
                text += codepoint.byte_size;

                if (!codepoint.character)
                    break;

                WMEventNode *event_n = push_struct(event_arena, WMEventNode);
                QueuePush(event_list->first, event_list->last, event_n);
                event_list->count++;

                WMEvent *event = &event_n->v;
                event->character = codepoint.character;
            }
            
        } break;
        }
    }

    editor_params.events = event_list;

    // TODO(fede): editor input
    editor.update_and_render(&editor_params, &render_group);

    glClearColor(0x0, 0x0, 0x0, 0xFF);
    glClear(GL_COLOR_BUFFER_BIT);

    // renderer_sync(&renderer);
    renderer_draw(&renderer, &render_group);

    SDL_GL_SwapWindow(window);

    {
        u64 end_counter = SDL_GetPerformanceCounter();

        f64 work_seconds_elapsed =
            sdl_get_seconds_elapsed(last_counter, end_counter);

        linux_sleep_to_target(last_counter, target_seconds_per_frame);

        f64 seconds_elapsed_for_frame =
            sdl_get_seconds_elapsed(last_counter, SDL_GetPerformanceCounter());

        last_counter = SDL_GetPerformanceCounter();

#if 0
        f64 ms_per_frame = seconds_elapsed_for_frame * 1000;
        f64 fps = 1000.0f / ms_per_frame;

        printf("%.02fms/f, %.02ffps \n", ms_per_frame, fps);
#endif
    }

    arena_clear(event_arena);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
