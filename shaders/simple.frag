#version 330 core

uniform sampler2D image;

in vec4 out_color;
in vec2 out_uv;

void main() {
    // gl_FragColor = vec4(out_uv, 0, 1);
    gl_FragColor = vec4(out_color.rgb, out_color.a * texture(image, out_uv).r);
}
