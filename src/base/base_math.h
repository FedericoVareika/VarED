#ifndef VARED_MATH_H
#define VARED_MATH_H

#define V2(x, y) ((v2){(x), (y)})
typedef struct {
    union {
        struct {
            f32 x, y;
        };
        f32 e[2];
    };
} v2;

typedef struct {
    union {
        struct {
            u32 x, y;
        };
        u32 e[2];
    };
} v2u;

#define V3(x, y, z) ((v3){(x), (y), (z)})
#define expand_v3(v) (v).x, (v).y, (v).z
typedef struct {
    union {
        struct {
            f32 x, y, z;
        };
        f32 e[3];
    };
} v3;

#define V4(x, y, z, w) ((v4){x, y, z, w})
typedef struct {
    union {
        struct {
            f32 x, y, z, w;
        };
        struct {
            f32 r, g, b, a;
        };
        struct {
            v2 xy, zw;
        };
        struct {
            v3 xyz;
            f32 w_;
        };
        f32 e[4];
    };
} v4;

typedef struct {
    union {
        struct {
            v2 min, max;
        };
        struct {
            v4 V4;
        };
    };
} Rect2;

internal inline v2 v2_add(v2 a, v2 b);
internal inline v2 v2_sub(v2 a, v2 b);
internal inline v2 v2_smul(v2 a, f32 m);
internal inline v2 v2_sdiv(v2 a, f32 m);
internal inline v2 v2_vmul(v2 a, v2 b);
internal inline v2 v2_neg(v2 a);
internal inline f32 v2_dot(v2 a, v2 b);
internal inline f32 v2_length2(v2 v);
internal inline v2 v2_reflect(v2 a, v2 normal, f32 bounce);
#define V2_INF (V2(F32_INF, F32_INF))
#define V2_NEG_INF (V2(-F32_INF, -F32_INF))

internal inline v2u v2u_add(v2u a, v2u b);
internal inline v2u v2u_sub(v2u a, v2u b);
internal inline v2u v2u_smul(v2u a, u32 m);
internal inline v2u v2u_sdiv(v2u a, u32 m);
internal inline v2u v2u_vmul(v2u a, v2u b);
internal inline v2u v2u_neg(v2u a);

internal inline v4 v4_add(v4 a, v4 b);
internal inline v4 v4_sub(v4 a, v4 b);
internal inline v4 v4_smul(v4 a, f32 m);
internal inline v4 v4_sdiv(v4 a, f32 m);
internal inline v4 v4_vmul(v4 a, v4 b);
internal inline v4 v4_neg(v4 a);
internal inline f32 v4_dot(v4 a, v4 b);
internal inline f32 v4_length2(v4 v);
internal inline v4 v4_lerp(v4 a, v4 b, f32 t);

internal inline v2 rect2_dim(Rect2 rect);
internal inline Rect2 rect2_min_max(v2 min, v2 max);
internal inline Rect2 rect2_min_dim(v2 min, v2 dim);
internal inline Rect2 rect2_center_halfdim(v2 center, v2 halfdim);
internal inline Rect2 rect2_center_dim(v2 center, v2 dim);
internal inline bool rect2_test_inside(Rect2 rect, v2 test);
internal inline bool rect2_overlap(Rect2 a, Rect2 b);
#define R2_INF ((Rect2){V2_NEG_INF, V2_INF})

#endif // VARED_MATH_H
