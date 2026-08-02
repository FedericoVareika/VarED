#ifndef BASE_BITMAP_H
#define BASE_BITMAP_H

typedef struct {
    u64 size;
    void *buf;
} Bitmap1d;

typedef struct {
    u64 width;
    u64 height;
    u64 size;

    u64 stride;
    u8 *buf;
} Bitmap2d;

#endif // BASE_BITMAP_H
