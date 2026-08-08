internal inline v2 v2_add(v2 a, v2 b) {
    return (v2){
        a.x + b.x,
        a.y + b.y,
    };
}

internal inline v2 v2_sub(v2 a, v2 b) {
    return (v2){
        a.x - b.x,
        a.y - b.y,
    };
}

internal inline v2 v2_smul(v2 a, f32 m) {
    return (v2){
        a.x * m,
        a.y * m,
    };
}

internal inline v2 v2_sdiv(v2 a, f32 m) {
    return (v2){
        a.x / m,
        a.y / m,
    };
}

internal inline v2 v2_vmul(v2 a, v2 b) {
    return (v2){
        a.x * b.x,
        a.y * b.y,
    };
}

internal inline v2 v2_neg(v2 a) {
    return (v2){
        -a.x,
        -a.y,
    };
}

internal inline f32 v2_dot(v2 a, v2 b) {
    return a.x * b.x + a.y * b.y;
}

internal inline f32 v2_length2(v2 v) {
    return v2_dot(v, v);
}

internal inline f32 square(f32 a) {
    return a * a;
}

internal inline v2 reflect(v2 a, v2 normal, f32 bounce) {
    return v2_add(a, v2_smul(normal, (1 + bounce) * v2_dot(a, v2_neg(normal))));
}

internal inline v2 rect2_dim(Rect2 rect) {
    return (v2){
        rect.max.x - rect.min.x,
        rect.max.y - rect.min.y,
    };
}

internal inline Rect2 rect2_min_max(v2 min, v2 max) {
    return (Rect2){
        .min = min,
        .max = max,
    };
}

internal inline Rect2 rect2_min_dim(v2 min, v2 dim) {
    return (Rect2){
        .min = min,
        .max = v2_add(min, dim),
    };
}

internal inline Rect2 rect2_center_halfdim(v2 center, v2 halfdim) {
    return (Rect2){
        .min = v2_sub(center, halfdim),
        .max = v2_add(center, halfdim),
    };
}

internal inline Rect2 rect2_center_dim(v2 center, v2 dim) {
    v2 halfdim = v2_smul(dim, 0.5);
    return rect2_center_halfdim(center, halfdim);
}

internal inline bool rect2_test_inside(Rect2 rect, v2 test) {
    return 
        rect.min.x <= test.x && 
        rect.min.y <= test.y && 
        rect.max.x > test.x && 
        rect.max.y > test.y;
}

internal inline v2u v2u_add(v2u a, v2u b) {
    return (v2u){
        a.x + b.x,
        a.y + b.y,
    };
}

internal inline v2u v2u_sub(v2u a, v2u b) {
    return (v2u){
        a.x - b.x,
        a.y - b.y,
    };
}

internal inline v2u v2u_smul(v2u a, u32 m) {
    return (v2u){
        a.x * m,
        a.y * m,
    };
}

internal inline v2u v2u_sdiv(v2u a, u32 m) {
    return (v2u){
        a.x / m,
        a.y / m,
    };
}

internal inline v2u v2u_vmul(v2u a, v2u b) {
    return (v2u){
        a.x * b.x,
        a.y * b.y,
    };
}

internal inline v2u v2u_neg(v2u a) {
    return (v2u){
        -a.x,
        -a.y,
    };
}

internal inline v4 v4_add(v4 a, v4 b) {
    return (v4) {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
        a.w + b.w,
    };
}

internal inline v4 v4_sub(v4 a, v4 b) {
    return (v4) {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
        a.w - b.w,
    };
}

internal inline v4 v4_smul(v4 a, f32 m) {
    return (v4) {
        a.x * m,
        a.y * m,
        a.z * m,
        a.w * m,
    };
}

internal inline v4 v4_sdiv(v4 a, f32 m) {
    return (v4) {
        a.x / m,
        a.y / m,
        a.z / m,
        a.w / m,
    };
}

internal inline v4 v4_vmul(v4 a, v4 b) {
    return (v4) {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z,
        a.w * b.w,
    };
}

internal inline v4 v4_neg(v4 a) {
    return (v4) {
        - a.x,
        - a.y,
        - a.z,
        - a.w,
    };
}

internal inline f32 v4_dot(v4 a, v4 b) {
    return 
        a.x * b.x +
        a.y * b.y + 
        a.z * b.z + 
        a.w * b.w;
}

internal inline f32 v4_length2(v4 v) {
    return v4_dot(v, v);
}

internal inline v4 v4_lerp(v4 a, v4 b, f32 t) {
    return v4_add(v4_smul(a, (1-t)), v4_smul(b, t));
}
