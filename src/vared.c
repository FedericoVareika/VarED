#include "base/base_inc.h"
#include "base/base_inc.c"

#include "vared.h"

#include "vared_renderer.h"
#include "vared_renderer.c"

#include <stdio.h>

internal Line line_alloc(Arena *arena, u32 size) {
    Line line = {0};
    line.buf = push_array(arena, u8, size);
    line.count = 0;
    line.size = size;
    return line;
}

internal void shift_at_cursor(Line *line, u32 cursor, int n) {
    assert(line->size >= line->count + n);
    assert(cursor <= line->count);
    assert(cursor + n < line->size);
    assert((i64)cursor + n >= 0);

    u8 *src = line->buf + cursor;
    u8 *dst = src + n;
    u64 count = line->count - cursor; 
    mem_move(dst, src, count);
    line->count += n;
}

internal void insert_char(Line *line, u32 *cursor, char c) {
    shift_at_cursor(line, *cursor, 1);
    line->buf[*cursor] = c;
    *cursor = *cursor + 1;
}

internal Line *new_line(Arena *arena, LineBuffer *text, u32 at) {
    assert(text->count < text->size);
    for (u32 i = text->count; i > at; i--) {
        text->lines[i] = text->lines[i - 1];
    }

    text->lines[at] = line_alloc(arena, kilobytes(1));
    text->count++;

    return &text->lines[at];
}

internal Vec2 render_unicode_line_in_text_window(
        Arena *arena,
        Font *font,
        RenderGroup *render_group,
        Line *line,
        Rect2 text_window,
        v2 text_offset) {

    FontCache *fc = font->cache; 
    FontProvider *fp = font->provider; 

    // NOTE(fede): Assume text_offset sets the baseline
    v2 text_origin = v2_add(text_window.min, text_offset);

    v2 text_pos  = {0};

    String8 line_str = str8(line->buf, line->count);
    while (line_str.size) {
        UnicodeCodepoint codepoint = utf8_decode(line_str.str, line_str.size);
        assert(codepoint.byte_size <= 4);

        // TODO(fede): 
        //      - Add support for multiple textures.
        //      - Render an entire line at once (Font Run)? 
        FontGlyph *glyph = fc_get_codepoint_glyph(fc, fp, codepoint);
        
        {
            v2 pos = v2_add(text_origin, text_pos);
            pos = v2_add(pos, (v2){
                glyph->bearing_x,
                -glyph->bearing_y, // check if neg or pos
            });
            v2 dim = {
                glyph->width,
                glyph->height,
            };

            Rect2 glyph_pos = rect2_min_dim(pos, dim);

            rg_push_textured_rect2(arena, render_group,
                    glyph_pos, (v4){1, 1, 1, 1}, glyph->uvs);
        }

        text_pos.x = glyph->advance; 

        // if (!glyph) {
        //     Temp scratch = begin_temp();
        //
        //     Bitmap2d fp_bitmap = fp_raster_codepoint_glyph(scratch.arena, fp, codepoint);
        //
        //     Bitmap2d atlas_dst = fc_
        //
        //     for (u64 row_pos = 0;
        //             row_pos < glyph_bitmap.size;
        //             row_pos += glyph_bitmap.stride) {
        //         u8 *row = (u8 *)glyph_bitmap.buf + row_pos;
        //         for (u64 col = 0; col < glyph_bitmap.width; col++) {
        //
        //         }
        //     }
        //
        //     glyph = fc_add_glyph(font->fc, glyph_info);
        //
        //
        //     // STUDY(fede): return something like "Buffer" type instead of 
        //     //      String8 which is confusing.
        //     String8 bitmap = fp_raster_glyph(scratch.arena, font->face, glyph_info);
        //
        //     fc_set_glyph_bitmap(font->fc, glyph, bitmap.str, bitmap.size);
        //
        //     end_temp(scratch);
        // }
    }
}

internal void render_line_in_rect2(
        EditorState *state,
        RenderGroup *render_group,
        Line line,
        u32 *cursor,
        Rect2 *window_rect,
        v4 color) {
    Font *font = &state->font;

    rg_push_set_shader(state->frame_arena, render_group, ShaderType_Text);

    f32 scale = font->font_height / (font->ascent - font->descent);

    stbtt_aligned_quad q;

    f32 current_x = window_rect->min.x;
    f32 current_y = window_rect->min.y; 
    current_y += scale * (f32)font->ascent;

    v2 cursor_pos = { current_x, current_y };

    u32 codepoint_idx = 0;
    String8 line_str = str8(line.buf, line.count);
    while (line_str.size) {
        UnicodeCodepoint codepoint = utf8_decode(line_str.str, line_str.size); 
        line_str = str8_skip(line_str, codepoint.byte_size);

        // TODO(fede): Use font cache to dynamically make/cache the bitmaps.

        u8 ascii = (u8)codepoint.character;
        if (codepoint.byte_size != 1 || codepoint.character < ' ' || codepoint.character > 127) {
            // fprintf(stderr, "Could not render non-ascii character: 0x%X\n", codepoint.character);
            continue;
        }

        int advance_width, left_side_bearing; 
        stbtt_GetCodepointHMetrics(&font->font_info, ascii, &advance_width, &left_side_bearing);

        if (codepoint_idx == 0) {
            current_x += scale * left_side_bearing;
        }

        if (current_x + scale * advance_width > window_rect->max.x) {
            current_x = window_rect->min.x + scale * left_side_bearing;
            current_y += scale * (f32)(font->y_increment);
            window_rect->min.y += scale * (f32)(font->y_increment);
        }

        stbtt_GetPackedQuad(
                font->char_data_for_range,
                font->pixels_width, font->pixels_height,
                ascii - font->first_char, 
                &current_x, &current_y, &q,
                true);

        if (current_y - scale * font->descent > window_rect->max.y)
            break;

        rg_push_textured_rect2(
                state->frame_arena,
                render_group, 
                rect2_min_max((v2){q.x0, q.y0}, (v2){q.x1, q.y1}),
                color,
                rect2_min_max((v2){q.s0, q.t0}, (v2){q.s1, q.t1}));

        // TODO(fede): Codepoint kerning with stbtt_GetKerningTable
        // if (line_idx < line.count) {
        //     current_x += scale * (f32)stbtt_GetCodepointKernAdvance(
        //             &font->font_info, *ascii, *(c + 1));
        // }
        
        if (cursor && codepoint_idx + 1 == *cursor) {
            cursor_pos = (v2){ current_x, current_y };
        }

        codepoint_idx++;
    }

    window_rect->min.y += scale * (f32)(font->y_increment);

    if (cursor) {
        rg_push_set_shader(
                state->frame_arena,
                render_group, ShaderType_Color);

        f32 cursor_width = 2;
        v2 cursor_upper_left = v2_sub(cursor_pos, (v2){cursor_width / 2, scale * font->ascent});

        rg_push_rect2(
                state->frame_arena,
                render_group, 
                rect2_min_dim(cursor_upper_left, (v2){cursor_width, font->font_height}),
                color);
    }
}

internal void editor_init(EditorParams *params, RenderGroup *render_group) {
    // TODO(fede): change to *params->memory = arena_bootstrap_struct(EditorState, arena)
    // STUDY(fede): change commit/reserve sizes for this
    Arena *arena = arena_alloc();
    EditorState *state = push_struct(arena, EditorState);
    state->arena = arena;
    *params->memory = state;

    // STUDY(fede): change commit/reserve sizes for this
    state->frame_arena = arena_alloc();

    PlatformApi platform = params->platform;

    Font *font = &state->font;
    {
        font->pixels_width = 512;
        font->pixels_height = 512;
        font->first_char = 32;
        font->num_chars = 96;
        font->font_height = 32; // NOTE(fede): pixels i think
        stbtt_fontinfo *font_info = &font->font_info;

        DebugReadFileResult font_file = platform.debug_platform_read_entire_file(
                0, "fonts/NotoSans/static/NotoSans_Condensed-Black.ttf");
        
        // 0, "fonts/Roboto/static/Roboto-Medium.ttf");
        // 0, "fonts/IosevkaTermNerdFontMono-Light.ttf");
        // 0, "fonts/IosevkaTermNerdFont-Light.ttf");

        assert(stbtt_InitFont(font_info, font_file.memory, 0));

        u8 *pixels = push_array(state->frame_arena, u8, 
                font->pixels_width * font->pixels_height);

        stbtt_pack_context spc;
        assert(stbtt_PackBegin(&spc,
                    pixels,
                    font->pixels_width, font->pixels_height,
                    0, 1,
                    0));

        font->char_data_for_range = push_array(
                state->arena, stbtt_packedchar, font->num_chars);
        assert(stbtt_PackFontRange(&spc,
                    font_file.memory,
                    0, font->font_height, 
                    font->first_char, font->num_chars,
                    font->char_data_for_range));

        stbtt_PackEnd(&spc);

        stbtt_GetFontVMetrics(&font->font_info,
                &font->ascent, &font->descent, &font->line_gap);
        font->y_increment = font->ascent - font->descent + font->line_gap;

        rg_push_texture_load(
                state->frame_arena,
                render_group, pixels,
                font->pixels_width, font->pixels_height);
    }

    // TODO(fede): Actual text data strutcture
    state->text.size = 1000;
    state->text.lines = push_array(state->arena, Line, state->text.size);
    state->text.count = 0;
    new_line(state->arena, &state->text, 0);
}

extern EDITOR_UPDATE_AND_RENDER(editor_update_and_render) {
    bool clear_frame_arena = true;
    if (!*params->memory) {
        editor_init(params, render_group);
        clear_frame_arena = false;
    }

    PlatformApi platform = params->platform;

    EditorState *state = (EditorState *)*params->memory;
    Arena *frame_arena = state->frame_arena;

    if (clear_frame_arena)
        arena_clear(frame_arena);

    state->text_window = rect2_center_dim(
            (v2){render_group->width / 2, render_group->height / 2},
            (v2){render_group->width - 20, render_group->height - 20});
            // (v2){render_group->width, 40 * state->font.font_height});

    Font font = state->font;
    int min_key = font.first_char;
    int max_key = font.first_char + font.num_chars;

    WMEventList *events = params->events;
    for (; events->first; QueuePop(events->first, events->last)) {
        WMEvent event = events->first->v;
        switch (event.key) {

        case WMKey_RETURN: {
            new_line(state->arena, &state->text, state->cursor_line + 1);
            state->cursor_line++;
            state->cursor_char = 0;
        } break;

        // TODO(fede): UTF8 movement, for now this just moves per-byte instead
        //      of per-codepoint.
        case WMKey_BACKSPACE: {
            Line *line = &state->text.lines[state->cursor_line];
            u32 n = event.repeat == 0 ? 1 : event.repeat;
            n = min(state->cursor_char, n);
            shift_at_cursor(line, state->cursor_char, -(int)n);
            state->cursor_char -= n;
        } break;

        case WMKey_LEFT: {
            if (state->cursor_char > 0) 
                state->cursor_char--;
        } break;
        case WMKey_RIGHT: {
            if (state->cursor_char < 
                    state->text.lines[state->cursor_line].count) 
                state->cursor_char++;
        } break;
        case WMKey_UP: {
            if (state->cursor_line > 0) {
                state->cursor_line--;
                state->cursor_char = 0;
            }
        } break;
        case WMKey_DOWN: {
            if (state->cursor_line + 1 < 
                    state->text.count) {
                state->cursor_line++;
                state->cursor_char = 0;
            }
        } break;

        case WMKey_o: {
            if ((event.modifiers & WMModifier_ctrl) && 
                !(event.modifiers & WMModifier_shift) &&
                !(event.modifiers & WMModifier_alt)) {
                Line line = state->text.lines[state->cursor_line];
                String8 path = str8(line.buf, line.count);

                // TODO(fede): Use scratch arena and implement pop
                char *path_cstr = cstr_from_str8(frame_arena, path);
                DebugReadFileResult file = platform.debug_platform_read_entire_file(0, path_cstr);

                if (!file.memory) {
                    printf("Could not open file: %s\n", path_cstr);
                    break;
                }

                Line *new = new_line(state->arena, &state->text, state->cursor_line + 1);
                state->cursor_line++;
                state->cursor_char = 0;

                u32 bytes_to_copy = min(file.size, new->size);
                mem_copy(new->buf, file.memory, bytes_to_copy);
                new->count = bytes_to_copy;
            }
        } break;

        case WMKey_l: {
            if ((event.modifiers & WMModifier_ctrl) && 
                !(event.modifiers & WMModifier_shift) &&
                !(event.modifiers & WMModifier_alt)) {
                Line line = state->text.lines[state->cursor_line];
                String8 line_str = str8(line.buf, line.count);
                char *line_cstr = cstr_from_str8(frame_arena, line_str);
                printf("%s\n", line_cstr);
            }
        } break;

        default: {
            if (event.character) {
                Line *line = &state->text.lines[state->cursor_line];
                u8 insert_chars[4] = {0};
                u32 codepoint_byte_size = utf8_encode(event.character, (u8 *)insert_chars);
                for (u32 byte_idx = 0;
                        byte_idx < codepoint_byte_size;
                        byte_idx++) {
                    insert_char(line, &state->cursor_char, insert_chars[byte_idx]);
                }
            }
        } break; 
        }
    }

    rg_push_set_shader(frame_arena, render_group, ShaderType_Color);
    rg_push_rect2(frame_arena, render_group, state->text_window, (v4){0.2, 0.2, 0.2, 1});

    Rect2 line_window = state->text_window;
    for (u32 line_idx = 0; line_idx < state->text.count; line_idx++) {
        u32 *cursor_char = state->cursor_line == line_idx ? &state->cursor_char : 0;
        render_line_in_rect2(
                state, 
                render_group,
                state->text.lines[line_idx],
                cursor_char,
                &line_window,
                (v4){0.8, 0.8, 0.8, 1});
    }
}
