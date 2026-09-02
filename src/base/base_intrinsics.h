#ifndef BASE_INTRINSICS_H
#define BASE_INTRINSICS_H

#include <math.h>

#define F32_INF INFINITY

internal inline int round_f32_to_int(f32 a) {
    return roundf(a);
    // return (int)(a + 0.5); 
}

internal inline int floor_f32_to_int(f32 a) {
    return floorf(a);
    // return a < 0 ? (int)(a)-1 : (int)(a); 
}

internal inline int ceil_f32_to_int(f32 a) {
    return ceilf(a);
    // return a < 0 ? (int)(a)-1 : (int)(a); 
}

internal inline int truncate_f32_to_int(f32 a) {
    return (int)(a);
}

internal inline f32 sin_f32(f32 angle) {
    return sinf(angle);
} 

internal inline f32 cos_f32(f32 angle) {
    return cosf(angle);
} 

internal inline f32 atan2_f32(f32 y, f32 x) {
    return atan2f(y, x);
} 

internal inline f32 abs_f32(f32 a) {
    return fabs(a);
} 

internal inline f32 sqrt_f32(f32 a) {
    return sqrtf(a);
}

internal inline int sign_int(int val) {
    return (val > 0) - (val < 0);
}

internal inline f32 pow_f32(f32 x, f32 y) {
    return powf(x, y);
}

internal inline f32 square_f32(f32 a) {
    return a * a;
}

internal inline f32 lerp_f32(f32 a, f32 b, f32 t) {
    return a * (1 - t) + b * t;
}

internal inline f32 ease_out_quint_f32(f32 x) {
    return 1 - pow_f32(1 - x, 5);
}

internal inline f32 ease_in_expo_f32(f32 x) {
    return x == 0 ? 0 : pow_f32(2, 10 * x - 10);
}

typedef struct {
    u32 index;
    bool found;
} FindBitResult; 

internal inline FindBitResult find_least_significant_set_bit(u32 mask) {
    FindBitResult result = {0};

#if COMPILER_GCC
    if (mask != 0) {
        result.index = __builtin_ctzll(mask);
        result.found = true;
    }
#elif COMPILER_CLANG
    // TODO(fede): do not know the clang intrinsic for this yet.
    while (result.index < 32) {
        if ((mask & (0x1 << result.index)) != 0) {
            result.found = true;
            return result;
        }

        result.index++;
    }
#else
    while (result.index < 32) {
        if ((mask & (0x1 << result.index)) != 0) {
            result.found = true;
            return result;
        }

        result.index++;
    }
#endif 

    return result;
}

#endif // BASE_INTRINSICS_H
