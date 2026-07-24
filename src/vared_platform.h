#ifndef VARED_PLATFORM_H
#define VARED_PLATFORM_H

/*
 * NOTE(fede): Compilers
*/
#ifdef __GNUC__
    #ifndef __clang__
        #define COMPILER_GCC 
    #else 
        #define COMPILER_CLANG 
    #endif //__clang__
#endif //__GNUC__

#include <stdint.h>
#include <stdio.h> // NOTE(fede): for size_t type

#define internal static
#define global static
#define local_persist static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef size_t MemoryIndex; 

typedef float f32;
typedef double f64;

// STUDY(fede): This is added because of SDL, check if this is necessary.
#if !defined(__bool_true_false_are_defined)
typedef enum { false, true } bool;
#endif

typedef struct {
    int placeholder;
} ThreadContext;

/*
 * NOTE(fede): These are platform services that are to be called from the
 * game layer.
 * */

#if VARED_INTERNAL

typedef struct {
    u64 size;
    void *memory;
} DebugReadFileResult;

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name)                                  \
    DebugReadFileResult name(ThreadContext *thread, char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile);

internal DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name)                                  \
    void name(ThreadContext *thread, DebugReadFileResult file_result)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeEntireFileMemory);

internal DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name)                                 \
    bool name(ThreadContext *thread, char *filename, u64 size, void *memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile);

internal DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif // VARED_INTERNAL

// NOTE(fede): Copied from SDL2 SDL_Keymod
typedef enum {
    KeymodFlag_none = 0x0000,
    KeymodFlag_lshift = 0x0001,
    KeymodFlag_rshift = 0x0002,
    KeymodFlag_lctrl = 0x0040,
    KeymodFlag_rctrl = 0x0080,
    KeymodFlag_lalt = 0x0100,
    KeymodFlag_ralt = 0x0200,
    KeymodFlag_lgui = 0x0400,
    KeymodFlag_rgui = 0x0800,
    KeymodFlag_num = 0x1000,
    KeymodFlag_caps = 0x2000,
    KeymodFlag_mode = 0x4000,
    KeymodFlag_scroll = 0x8000,

    KeymodFlag_ctrl = KeymodFlag_lctrl | KeymodFlag_rctrl,
    KeymodFlag_shift = KeymodFlag_lshift | KeymodFlag_rshift,
    KeymodFlag_alt = KeymodFlag_lalt | KeymodFlag_ralt,
    KeymodFlag_gui = KeymodFlag_lgui | KeymodFlag_rgui,

    KeymodFlag_reserved = KeymodFlag_scroll,
} KeymodFlag;

typedef struct {
    i32 key;
    i32 repeat;
    KeymodFlag mod;
} KeyInput;

typedef struct {
    KeyInput key_inputs[512];
    u32 key_input_count;
} EditorInput;

typedef struct {
    bool is_initialized;

    MemoryIndex permanent_storage_size;
    void *permanent_storage;

    MemoryIndex transient_storage_size;
    void *transient_storage;

#if VARED_INTERNAL
    DEBUGPlatformReadEntireFile *debug_platform_read_entire_file;
    DEBUGPlatformFreeEntireFileMemory *debug_platform_free_file_memory;
    DEBUGPlatformWriteEntireFile *debug_platform_write_entire_file;
#endif

} EditorMemory;

#include "vared_renderer.h"
    
#define EDITOR_UPDATE_AND_RENDER(name) \
    void name(EditorMemory *memory, EditorInput *input, RenderGroup *render_group)
typedef EDITOR_UPDATE_AND_RENDER(EditorUpdateAndRender);

extern EDITOR_UPDATE_AND_RENDER(editor_update_and_render);

#endif // VARED_PLATFORM_H

