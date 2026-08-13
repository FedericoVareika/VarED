#version 330 core

uniform vec2 screen_resolution;

layout (location = 0) in vec4 i_pos_rect;
layout (location = 1) in vec4 i_uv_rect;
layout (location = 2) in vec4 i_clip_rect;
layout (location = 3) in vec4 i_colors[4];
layout (location = 7) in float i_corner_radius;
layout (location = 8) in float i_edge_softness;
layout (location = 9) in float i_border_thickness;
layout (location = 10) in float i_ignore_texture;

out vec4 out_color;
out vec2 out_uv;
out vec2 out_pos;
out vec2 out_center;
out vec2 out_half_size;
out vec4 out_clip_rect;

out float out_corner_radius; 
out float out_edge_softness;
out float out_border_thickness;
out float out_ignore_texture;

void main() {
    vec2 local_positions[] = vec2[](
            vec2(-1, -1),
            vec2(-1, +1),
            vec2(+1, -1),
            vec2(+1, +1));

    vec2 dst_half_size = (i_pos_rect.zw - i_pos_rect.xy) / 2;
    vec2 dst_center = (i_pos_rect.zw + i_pos_rect.xy) / 2;
    vec2 dst_pos =
        (local_positions[gl_VertexID] * dst_half_size + dst_center);

    vec2 uv_half_size = (i_uv_rect.zw - i_uv_rect.xy) / 2;
    vec2 uv_center = (i_uv_rect.zw + i_uv_rect.xy) / 2;
    vec2 uv_pos =
        (local_positions[gl_VertexID] * uv_half_size + uv_center);

    vec2 ndc = 2 * dst_pos / screen_resolution - 1;
    ndc.y *= -1;
    gl_Position = vec4(ndc, 0, 1);

    out_uv = uv_pos;
    out_color = i_colors[gl_VertexID];

    out_pos = dst_pos;
    out_center = dst_center;
    out_half_size = dst_half_size;

    out_clip_rect = i_clip_rect;

    out_corner_radius = i_corner_radius;
    out_edge_softness = i_edge_softness;
    out_border_thickness = i_border_thickness;
    out_ignore_texture = i_ignore_texture;
}
