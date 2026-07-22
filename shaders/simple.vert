#version 330 core

uniform vec2 screen_resolution;

layout(location=0) in vec3 position;
layout(location=1) in vec4 color;
layout(location=2) in vec2 uv;

out vec4 out_color;
out vec2 out_uv;

void main() {
    vec2 normalized = position.xy / screen_resolution;
    vec2 ndc = normalized * 2 - 1;
    ndc.y *= -1;

    gl_Position = vec4(ndc, position.z, 1);
    out_color = color;
    out_uv = uv;
}
