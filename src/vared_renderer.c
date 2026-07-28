#include "vared_renderer.h"

internal RenderEntryNode *rg_push_entry_n(
        Arena *arena,
        RenderEntryList *entry_list,
        RenderEntryType entry_type) {

    RenderEntryNode *entry_n = push_struct(arena, RenderEntryNode);
    entry_n->v.type = entry_type;

    QueuePush(entry_list->first, entry_list->last, entry_n);
    entry_list->count++;
    return entry_n;
}

internal void rg_push_textured_quad(
        Arena *arena,
        RenderGroup *render_group,
        v3 p0, v3 p1, v3 p2, v3 p3, v4 color, v2 uv0, v2 uv1, v2 uv2, v2 uv3) {
    RenderEntryNode *entry_n = rg_push_entry_n(
            arena, &render_group->entries, RenderEntryType_Quad);
    RenderEntryQuad *quad_entry = &entry_n->v.quad;

    /* Quad vertices:
     *      0-1
     *      |\|
     *      2-3
     * */
    Vertex *vertices = quad_entry->vertices;
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

    u16 *indices = quad_entry->indices;
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 1;
    indices[4] = 3;
    indices[5] = 2;
}

internal void rg_push_quad(
        Arena *arena,
        RenderGroup *render_group,
        v3 p0, v3 p1, v3 p2, v3 p3, v4 color) {

    RenderEntryNode *entry_n = rg_push_entry_n(
            arena, &render_group->entries, RenderEntryType_Quad);
    RenderEntryQuad *quad_entry = &entry_n->v.quad;

    /* Quad vertices:
     *      0-1
     *      |\|
     *      2-3
     * */
    Vertex *vertices = quad_entry->vertices;
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

    u16 *indices = quad_entry->indices;
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 1;
    indices[4] = 3;
    indices[5] = 2;
}

internal void rg_push_rect2(
        Arena *arena,
        RenderGroup *render_group, Rect2 rect, v4 color) {
    rg_push_quad(
            arena,
            render_group,
            (v3){rect.min.x, rect.min.y, 0},
            (v3){rect.max.x, rect.min.y, 0},
            (v3){rect.min.x, rect.max.y, 0},
            (v3){rect.max.x, rect.max.y, 0}, color);
}

internal void rg_push_textured_rect2(
        Arena *arena,
        RenderGroup *render_group,
        Rect2 pos_rect, v4 color, Rect2 uv_rect) {
    rg_push_textured_quad(
            arena,
            render_group,
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

internal void rg_push_texture_load(
        Arena *arena,
        RenderGroup *render_group,
        u8 *data, u32 width, u32 height) {
    RenderEntryNode *entry_n = rg_push_entry_n(
            arena, &render_group->entries, RenderEntryType_TextureLoad);
    RenderEntryTextureLoad *texture_load = &entry_n->v.texture_load;
    texture_load->data = data;
    texture_load->width = width;
    texture_load->height = height;
}

internal void rg_push_set_shader(
        Arena *arena,
        RenderGroup *render_group,
        ShaderType shader) {
    assert(shader < RenderEntryType_Count);

    RenderEntryNode *entry_n = rg_push_entry_n(
            arena, &render_group->entries, RenderEntryType_ShaderSet);
    RenderEntryShaderSet *shader_set = &entry_n->v.shader_set;
    shader_set->shader = shader;
}
