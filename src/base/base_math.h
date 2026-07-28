#ifndef VARED_MATH_H
#define VARED_MATH_H

typedef struct {
    union {
        struct {
            f32 x, y;
        };
        f32 e[2];
    };
} v2;

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

typedef struct {
    union {
        struct {
            f32 x, y, z;
        };
        f32 e[3];
    };
} v3;

typedef struct {
    union {
        struct {
            f32 x, y, z, w;
        };
        struct {
            f32 r, g, b, a;
        };
        f32 e[4];
    };
} v4;

typedef struct {
    v2 min, max;
} Rect2;

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

internal inline bool inside_rect2(Rect2 rect, v2 test) {
    return 
        rect.min.x <= test.x && 
        rect.min.y <= test.y && 
        rect.max.x > test.x && 
        rect.max.y > test.y;
}

#endif // VARED_MATH_H
