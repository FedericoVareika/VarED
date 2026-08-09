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
in float out_border_thickness;
in float out_ignore_texture;

float rounded_rect_sdf(
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

    float dist = rounded_rect_sdf(
            out_pos,
            out_center,
            out_half_size - softness_padding,
            out_corner_radius);

    float sdf_factor = 1.f - smoothstep(0, 2*softness, dist);

    float border_factor = 1.f;
    if (out_border_thickness != 0) {
        vec2 interior_half_size =
            out_half_size - vec2(out_border_thickness);

        float interior_radius_reduce_f = 
            min(interior_half_size.x / out_half_size.x,
                    interior_half_size.y / out_half_size.y);
        float interior_corner_radius =
            (out_corner_radius *
             interior_radius_reduce_f *
             interior_radius_reduce_f);

        float inside_d = rounded_rect_sdf(
                out_pos,
                out_center,
                interior_half_size - softness_padding,
                interior_corner_radius);

        float inside_f = smoothstep(0, 2*softness, inside_d);
        border_factor = inside_f;
    }

    vec4 texture_sample = vec4(1.f, 1.f, 1.f, 1.f);
    if (out_ignore_texture < 1) {
        texture_sample = texture(image, out_uv);
    }

    gl_FragColor = out_color * texture_sample * sdf_factor * border_factor;
}
