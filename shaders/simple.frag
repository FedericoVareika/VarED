#version 330 core

uniform sampler2D image;

in vec4 out_color;
in vec2 out_uv;

// NOTE(fede): In -- Rounded corners
in vec2 out_pos;      
in vec2 out_center;
in vec2 out_half_size;
in float out_corner_radius; 
in float out_edge_softness;

float rounded_corners_sdf(
        vec2 sample_pos,
        vec2 rect_center,
        vec2 rect_half_size,
        float r) {
  vec2 d2 = (abs(rect_center - sample_pos) -
             rect_half_size +
             vec2(r, r));
  return min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - r;
}

void main() {
    float softness = out_edge_softness;
    vec2 softness_padding = vec2(max(0, softness*2-1),
                                     max(0, softness*2-1));

    float dist = rounded_corners_sdf(out_pos,
                            out_center,
                            out_half_size - softness_padding,
                            out_corner_radius);

    float sdf_factor = 1.f - smoothstep(0, 2*softness, dist);

    gl_FragColor = out_color * texture(image, out_uv) * sdf_factor;
}
