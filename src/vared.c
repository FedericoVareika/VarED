#include "vared.h"

#include "vared_renderer.h"
#include "vared_renderer.c"

#include <stdio.h>

extern EDITOR_UPDATE_AND_RENDER(editor_update_and_render) {
    EditorState *editor_state = (EditorState *)memory->permanent_storage;

    Arena frame_arena = {0};
    initialize_arena(
            &frame_arena,
            memory->transient_storage_size,
            memory->transient_storage);

    {
        u8 *push_buffer = push_size(&frame_arena, megabytes(4));
        initialize_arena(&render_group->push_buffer_arena, megabytes(4), push_buffer);
        render_group->vertex_buffer_arena.used = 0;
        render_group->index_buffer_arena.used = 0;
    }

    if (!memory->is_initialized) {
        initialize_arena(&editor_state->arena,
                memory->permanent_storage_size - sizeof(EditorState),
                (u8 *)memory->permanent_storage + sizeof(EditorState));
        render_group->push_buffer_arena.size = megabytes(4);
        render_group->push_buffer_arena.base = push_size(
                &editor_state->arena,
                render_group->push_buffer_arena.size);

        Font *font = &editor_state->font;
        {
            font->num_chars = 96;
            font->pixel_height = 32.0f;
            font->bitmap_width = 256;
            font->bitmap_height = 256;

            font->cdata = push_array(
                    &editor_state->arena,
                    stbtt_bakedchar,
                    font->num_chars);

            u8 *temp_bitmap = push_array(
                    &frame_arena, 
                    u8, font->bitmap_width * font->bitmap_height);

            DebugReadFileResult file = memory->debug_platform_read_entire_file(
                    0, "fonts/IosevkaTermNerdFontMono-Light.ttf");

            int bake_result = stbtt_BakeFontBitmap(
                    file.memory, 0,
                    font->pixel_height,
                    temp_bitmap,
                    font->bitmap_width, font->bitmap_height,
                    32, font->num_chars,
                    font->cdata); 

            printf("bake result: %d\n", bake_result);

            memory->debug_platform_free_file_memory(0, file);

            // TODO(fede): When we have multiple of these, we need to 
            //      distinguish them. We can use a hash map, or local id's or 
            //      something.
            rg_push_texture_load(
                    render_group,
                    temp_bitmap,
                    font->bitmap_width,
                    font->bitmap_height);

            memory->debug_platform_write_entire_file(0, "fonts/bitmap.bin", font->bitmap_width * font->bitmap_height, temp_bitmap);
        }


        memory->is_initialized = true;
    }

    char *hello = "hola mama!";

    stbtt_aligned_quad q;
    float current_x = 500;
    float current_y = render_group->height / 2;
    for (char *c = hello; *c; c++) {
        stbtt_GetBakedQuad(editor_state->font.cdata,
                editor_state->font.bitmap_width, editor_state->font.bitmap_height,
                *c - 32, 
                &current_x, &current_y, &q,
                true);

        rg_push_textured_rect2(render_group, 
                rect2_min_max((v2){q.x0, q.y0}, (v2){q.x1, q.y1}),
                (v4){0, 1, 1, 1}, 
                rect2_min_max((v2){q.s0, q.t0}, (v2){q.s1, q.t1}));
    }
}
