#include "vared.h"

#include "vared_renderer.h"
#include "vared_renderer.c"

#include <stdio.h>

extern EDITOR_UPDATE_AND_RENDER(editor_update_and_render) {
    EditorState *editor_state = (EditorState *)memory->permanent_storage;

    if (!memory->is_initialized) {
        initialize_arena(&editor_state->arena,
                memory->permanent_storage_size - sizeof(EditorState),
                (u8 *)memory->permanent_storage + sizeof(EditorState));
        render_group->push_buffer_base = push_size(&editor_state->arena, megabytes(4));

        memory->is_initialized = true;
    }

    // TODO(fede): Maybe allocate separately, maybe this should not be 
    //      dependent on game code.
    //      Also do not know if this should be done every frame.
    initialize_arena(
            &render_group->push_buffer_arena,
            megabytes(4),
            render_group->push_buffer_base);
    render_group->vertex_buffer_arena.used = 0;
    render_group->index_buffer_arena.used = 0;

    rg_push_rect2(render_group, rect2_min_max(
                (v2){-0.5, -0.5},
                (v2){0, 0}), (v4){0, 1, 1, 1});

    rg_push_rect2(render_group, rect2_min_max(
                (v2){0, 0},
                (v2){0.5, 0.5}), (v4){1, 0, 1, 1});
}
