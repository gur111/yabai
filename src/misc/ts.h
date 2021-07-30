#ifndef TS_H
#define TS_H

static struct {
    void *memory;
    uint64_t size;
    volatile uint64_t used;
} g_temp_storage;

bool ts_init(uint64_t size)
{
    uint64_t num_pages = size / PAGE_SIZE;
    uint64_t remainder = size % PAGE_SIZE;
    if (remainder) num_pages++;

    g_temp_storage.used = 0;
    g_temp_storage.size = num_pages * PAGE_SIZE;
    g_temp_storage.memory = mmap(0, g_temp_storage.size + PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

    bool result = g_temp_storage.memory != MAP_FAILED;
    if (result) mprotect(g_temp_storage.memory + g_temp_storage.size, PAGE_SIZE, PROT_NONE);

    return result;
}

static inline uint64_t ts_align(uint64_t used, uint64_t align)
{
    assert((align & (align-1)) == 0);

    uintptr_t ptr   = (uintptr_t)(g_temp_storage.memory + used);
    uintptr_t a_ptr = (uintptr_t) align;
    uintptr_t mod   = ptr & (a_ptr - 1);

    if (mod != 0) ptr += a_ptr - mod;

    return ptr - (uintptr_t)g_temp_storage.memory;
}

static inline void *ts_alloc(uint64_t size)
{
    for (;;) {
        uint64_t used = g_temp_storage.used;
        uint64_t aligned = ts_align(used, 16);
        uint64_t new_used = aligned + size;

        if (__sync_bool_compare_and_swap(&g_temp_storage.used, used, new_used)) {
            return g_temp_storage.memory + aligned;
        }
    }
}

static inline void *ts_expand(void *ptr, uint64_t old_size, uint64_t increment)
{
    if (!ptr) return ts_alloc(increment);

    assert(ptr == g_temp_storage.memory + g_temp_storage.used - old_size);
    __sync_fetch_and_add(&g_temp_storage.used, increment);

    return ptr;
}

static inline void ts_reset(void)
{
    g_temp_storage.used = 0;
}

#endif
