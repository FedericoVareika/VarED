
#include <sys/sysinfo.h>

internal u32 t_get_n_logical_cores(void) {
    return get_nprocs();
}
