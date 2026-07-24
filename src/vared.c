#include "vared.h"

#include "vared_renderer.h"
#include "vared_renderer.c"

#include <stdio.h>

internal void render_s8_in_rect2(
        RenderGroup *render_group,
        EditorState *editor_state,
        S8 string,
        Rect2 window_rect,
        v4 color) {
    Font *font = &editor_state->font; 

    f32 scale = font->font_height / (font->ascent - font->descent);

    stbtt_aligned_quad q;

    f32 current_x = window_rect.min.x;
    f32 current_y = window_rect.min.y; 
    current_y += scale * (f32)font->ascent;

    char *c = (char *)string.buf;
    for (u32 string_idx = 0;
            string_idx < string.count; 
            string_idx++, c++) {
        int advance_width, left_side_bearing; 
        stbtt_GetCodepointHMetrics(&font->font_info, *c, &advance_width, &left_side_bearing);

        if (string_idx == 0) {
            current_x += scale * left_side_bearing;
        }

        if (current_x + scale * advance_width > editor_state->text_window.max.x) {
            current_x = window_rect.min.x + scale * left_side_bearing;
            current_y += scale * (f32)(font->y_increment);
        }

        stbtt_GetPackedQuad(
                font->char_data_for_range,
                font->pixels_width, font->pixels_height,
                *c - font->first_char, 
                &current_x, &current_y, &q,
                true);

        if (current_y - scale * font->descent > window_rect.max.y)
            break;

        rg_push_textured_rect2(render_group, 
                rect2_min_max((v2){q.x0, q.y0}, (v2){q.x1, q.y1}),
                color,
                rect2_min_max((v2){q.s0, q.t0}, (v2){q.s1, q.t1}));

        // if (string_idx < string.count) {
        //     current_x += scale * (f32)stbtt_GetCodepointKernAdvance(&font->font_info, *c, *(c + 1));
        // }
    }
}

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
            font->pixels_width = 512;
            font->pixels_height = 512;
            font->first_char = 32;
            font->num_chars = 96;
            font->font_height = 64; // NOTE(fede): pixels i think
            stbtt_fontinfo *font_info = &font->font_info;

            DebugReadFileResult font_file = memory->debug_platform_read_entire_file(
                    0, "fonts/IosevkaTermNerdFontMono-Light.ttf");

            assert(stbtt_InitFont(font_info, font_file.memory, 0));

            u8 *pixels = push_array(&frame_arena, u8, 
                    font->pixels_width * font->pixels_height);

            stbtt_pack_context spc;
            assert(stbtt_PackBegin(&spc,
                    pixels,
                    font->pixels_width, font->pixels_height,
                    0, 1,
                    0));

            font->char_data_for_range = push_array(
                    &editor_state->arena, stbtt_packedchar, font->num_chars);
            assert(stbtt_PackFontRange(&spc,
                font_file.memory,
                0, font->font_height, 
                font->first_char, font->num_chars,
                font->char_data_for_range));

            stbtt_PackEnd(&spc);

            stbtt_GetFontVMetrics(&font->font_info,
                    &font->ascent, &font->descent, &font->line_gap);
            font->y_increment = font->ascent - font->descent + font->line_gap;

            rg_push_texture_load(render_group, pixels,
                    font->pixels_width, font->pixels_height);
        }

        editor_state->string.size = megabytes(4);
        editor_state->string.buf = push_size(&editor_state->arena, editor_state->string.size);
        editor_state->string.count = 0;

        memory->is_initialized = true;
    }

    editor_state->text_window = rect2_center_dim(
            (v2){render_group->width / 2, render_group->height / 2},
            (v2){500, 2 * editor_state->font.font_height});

    Font font = editor_state->font;
    int min_key = font.first_char;
    int max_key = font.first_char + font.num_chars;
    for (u32 i = 0; i < input->key_input_count; i++) {
        KeyInput key_input = input->key_inputs[i]; 
        if (key_input.key >= min_key && key_input.key < max_key) {
            u8 c = (u8)key_input.key;
            assert((int)c == key_input.key);

            for (int j = 0; j < (key_input.repeat ? key_input.repeat : 1); j++) {
                assert(editor_state->string.size > editor_state->string.count);
                editor_state->string.buf[editor_state->string.count++] = c;
            }
        }
    }

    rg_push_set_shader(render_group, ShaderType_Color);
    rg_push_rect2(render_group, editor_state->text_window, (v4){0.2, 0.2, 0.2, 1});


    // rg_push_set_shader(render_group, ShaderType_Color);
    // render_s8_in_rect2(
    //         render_group,
    //         editor_state,
    //         editor_state->string,
    //         editor_state->text_window,
    //         (v4){0.5, 0.5, 0.5, 1});

    rg_push_set_shader(render_group, ShaderType_Text);
    render_s8_in_rect2(
            render_group,
            editor_state,
            editor_state->string,
            editor_state->text_window,
            (v4){0.8, 0.8, 0.8, 1});
}
