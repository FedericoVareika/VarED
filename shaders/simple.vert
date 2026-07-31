#version 330 core

uniform vec2 screen_resolution;

layout(location=0) in vec4 i_pos_rect;
layout(location=1) in vec4 i_uv_rect;
layout(location=2) in vec4 i_color;

out vec4 out_color;
out vec2 out_uv;

void main() {
    vec2 local_positions[] = vec2[](vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1));
    vec2 local_pos = local_positions[gl_VertexID];

    vec2 corner_pos = i_pos_rect.xy + (local_pos * i_pos_rect.zw);
    vec2 uv = mix(i_uv_rect.xy, i_uv_rect.zw, local_pos);

    vec2 normalized = corner_pos / screen_resolution;
    vec2 ndc = normalized * 2 - 1;
    ndc.y *= -1;

    gl_Position = vec4(ndc, 0, 1);
    out_color = i_color;
    out_uv = uv;
}
