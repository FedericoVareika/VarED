#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

// NOTE(fede): This is copied from raddbg, basically the whole base file 
//          structure is aswell.

internal void *mem_reserve(u64 size);
internal bool mem_commit(void *base, u64 size);
internal bool mem_decommit(void *base, u64 size);
internal bool mem_release(void *base, u64 size);

#include <string.h>

#define mem_set(base, v, count) memset(base, v, count)
#define mem_zero(base, count) memset(base, 0, count)
#define mem_copy(dst, src, count) memcpy(dst, src, count)
#define mem_move(dst, src, count) memmove(dst, src, count)

#endif // BASE_MEMORY_H
