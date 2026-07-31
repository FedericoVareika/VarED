#ifndef LINUX_VARED_H
#define LINUX_VARED_H

#include <sys/stat.h>

#include <limits.h>
#define LINUX_FILEPATH_MAX_COUNT PATH_MAX

typedef struct {
    u64 editor_memory_size;
    void *editor_memory_block; 

    int recording_fd;
    int recording_index; 

    int playback_fd;
    int playing_index; 

    u64 bytes_written;
    u64 bytes_read;
    u64 memory_map_size;
    void *memory_map;

    char exe_filename[LINUX_FILEPATH_MAX_COUNT];
    char *one_past_last_slash;
} LinuxState;

#endif // LINUX_VARED_H
