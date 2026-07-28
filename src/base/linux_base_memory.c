internal void *mem_reserve(u64 size) {
    void *base = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return base;
}

internal bool mem_commit(void *base, u64 size) {
    int result = mprotect(base, size, PROT_READ | PROT_WRITE);
    return result == 0;
}

internal bool mem_decommit(void *base, u64 size) {
    int result = madvise(base, size, MADV_DONTNEED);
    result += mprotect(base, size, PROT_NONE);
    return result == 0;
}

internal bool mem_release(void *base, u64 size) {
    int result = munmap(base, size);
    return result == 0;
}
