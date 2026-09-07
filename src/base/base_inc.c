#include "base_core.c"
#include "base_arena.c"
#include "base_math.c"
#include "base_string.c"

#if OS_LINUX
#include "linux_base_memory.c"
#include "linux_base_perf.c"
#endif
