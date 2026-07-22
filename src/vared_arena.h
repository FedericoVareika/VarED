#ifndef VARED_ARENA_H
#define VARED_ARENA_H

#include "vared.h"

typedef struct {
    MemoryIndex size;
    MemoryIndex used;

    u8 *base;
} Arena;

#endif // VARED_ARENA_H

