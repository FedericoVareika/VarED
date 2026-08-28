#include <time.h>

i64 performance_counter(void)
{
    i64 ticks = 0;
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    ticks = now.tv_sec;
    ticks *= 1000000000;
    ticks += now.tv_nsec;

    return ticks;
}

u64 performance_frequency(void)
{
    return 1000000000;
}

