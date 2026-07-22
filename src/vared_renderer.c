#include "vared_renderer.h"

void rg_push_quad(RenderGroup *render_group,
        v3 p0, v3 p1, v3 p2, v3 p3, v4 color) {

    RenderEntryHeader *header = push_struct(&render_group->push_buffer_arena, RenderEntryHeader);
    header->type = RenderEntryType_Quad; 

    RenderEntryQuad *quad_entry = push_struct(&render_group->push_buffer_arena, RenderEntryQuad);
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

    u16 *indices = push_array(&render_group->index_buffer_arena, u16, 6); 
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 1;
    indices[4] = 3;
    indices[5] = 2;
}

void rg_push_rect2(RenderGroup *render_group, Rect2 rect, v4 color) {
    rg_push_quad(render_group,
            (v3){rect.min.x, rect.min.y, 0},
            (v3){rect.max.x, rect.min.y, 0},
            (v3){rect.min.x, rect.max.y, 0},
            (v3){rect.max.x, rect.max.y, 0}, color);
}

void rg_init_vertex_index_buffers(RenderGroup *render_group, Renderer *renderer) {
    render_group->vertex_buffer_ = renderer->vertex_buffer;
    initialize_arena(
            &render_group->vertex_buffer_arena,
            renderer->vertex_count * sizeof(Vertex),
            (u8 *)render_group->vertex_buffer_);

    render_group->index_buffer_ = renderer->index_buffer;
    initialize_arena(
            &render_group->index_buffer_arena,
            renderer->index_count * sizeof(u16),
            (u8 *)render_group->index_buffer_);
}
