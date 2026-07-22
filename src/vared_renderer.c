#include "vared_renderer.h"

#define rg_push_entry(render_group, type) (RenderEntry##type *)rg_push_entry_(render_group, sizeof(RenderEntry##type), RenderEntryType_##type)
internal void *rg_push_entry_(
        RenderGroup *render_group,
        size_t entry_size,
        RenderEntryType entry_type) {

    RenderEntryHeader *header = push_struct(&render_group->push_buffer_arena, RenderEntryHeader);
    header->type = entry_type; 

    return push_size(&render_group->push_buffer_arena, entry_size);
}

internal void rg_push_textured_quad(RenderGroup *render_group,
        v3 p0, v3 p1, v3 p2, v3 p3, v4 color, v2 uv0, v2 uv1, v2 uv2, v2 uv3) {
    RenderEntryQuad *quad_entry = rg_push_entry(render_group, Quad);
    quad_entry->vertex_offset = render_group->vertex_buffer_arena.used / sizeof(Vertex);
    quad_entry->index_offset = render_group->index_buffer_arena.used / sizeof(u16);

    /* Quad vertices:
     *      0-1
     *      |\|
     *      2-3
     * */
    Vertex *vertices = push_array(&render_group->vertex_buffer_arena, Vertex, 4); 
    vertices[0].position = p0;
    vertices[1].position = p1;
    vertices[2].position = p2;
    vertices[3].position = p3;

    vertices[0].color = color;
    vertices[1].color = color;
    vertices[2].color = color;
    vertices[3].color = color;

    vertices[0].uv = uv0;
    vertices[1].uv = uv1;
    vertices[2].uv = uv2;
    vertices[3].uv = uv3;

    u16 *indices = push_array(&render_group->index_buffer_arena, u16, 6); 
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 1;
    indices[4] = 3;
    indices[5] = 2;
}

internal void rg_push_quad(RenderGroup *render_group,
        v3 p0, v3 p1, v3 p2, v3 p3, v4 color) {
    RenderEntryQuad *quad_entry = rg_push_entry(render_group, Quad);
    quad_entry->vertex_offset = render_group->vertex_buffer_arena.used / sizeof(Vertex);
    quad_entry->index_offset = render_group->index_buffer_arena.used / sizeof(u16);

    /* Quad vertices:
     *      0-1
     *      |\|
     *      2-3
     * */
    Vertex *vertices = push_array(&render_group->vertex_buffer_arena, Vertex, 4); 
    vertices[0].position = p0;
    vertices[1].position = p1;
    vertices[2].position = p2;
    vertices[3].position = p3;
    vertices[0].color = color;
    vertices[1].color = color;
    vertices[2].color = color;
    vertices[3].color = color;

    vertices[0].uv = (v2){0, 0};
    vertices[1].uv = (v2){0, 255};
    vertices[2].uv = (v2){255, 0};
    vertices[3].uv = (v2){255, 255};

    u16 *indices = push_array(&render_group->index_buffer_arena, u16, 6); 
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 1;
    indices[4] = 3;
    indices[5] = 2;
}

internal void rg_push_rect2(RenderGroup *render_group, Rect2 rect, v4 color) {
    rg_push_quad(render_group,
            (v3){rect.min.x, rect.min.y, 0},
            (v3){rect.max.x, rect.min.y, 0},
            (v3){rect.min.x, rect.max.y, 0},
            (v3){rect.max.x, rect.max.y, 0}, color);
}

internal void rg_push_textured_rect2(RenderGroup *render_group, Rect2 pos_rect, v4 color, Rect2 uv_rect) {
    rg_push_textured_quad(render_group,
            (v3){pos_rect.min.x, pos_rect.min.y, 0},
            (v3){pos_rect.max.x, pos_rect.min.y, 0},
            (v3){pos_rect.min.x, pos_rect.max.y, 0},
            (v3){pos_rect.max.x, pos_rect.max.y, 0},
            color,
            (v2){uv_rect.min.x, uv_rect.min.y},
            (v2){uv_rect.max.x, uv_rect.min.y},
            (v2){uv_rect.min.x, uv_rect.max.y},
            (v2){uv_rect.max.x, uv_rect.max.y});
}

internal void rg_push_texture_load(RenderGroup *render_group, u8 *data, u32 width, u32 height) {
    RenderEntryTextureLoad *texture_load = rg_push_entry(render_group, TextureLoad);
    texture_load->data = data;
    texture_load->width = width;
    texture_load->height = height;
}

internal void rg_init_vertex_index_buffers(RenderGroup *render_group, Renderer *renderer) {
    initialize_arena(
            &render_group->vertex_buffer_arena,
            renderer->vertex_count * sizeof(Vertex),
            (u8 *)renderer->vertex_buffer);

    initialize_arena(
            &render_group->index_buffer_arena,
            renderer->index_count * sizeof(u16),
            (u8 *)renderer->index_buffer);
}
