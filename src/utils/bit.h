#ifndef SRC_UTILS_BIT_H
#define SRC_UTILS_BIT_H

#include <stdint.h>
#include <stddef.h>

static inline void bit_set(uint8_t *set, size_t index)
{
    set[index >> 3] |= (1 << (index & 7));
}

static inline int bit_check(uint8_t *set, size_t index)
{
    return set[index >> 3] & (1 << (index & 7));
}

static inline void bit_clear(uint8_t *set, size_t index)
{
    set[index >> 3] &= ~(1 << (index & 7));
}

#include "arena.h"
#include <string.h>

static inline uint8_t* bit_new(Arena *arena, size_t num)
{
    size_t byte = (num << 3) + 1;
    uint8_t *set = arena_malloc(arena, byte);
    memset(set, 0, byte);
    return set;
}

static inline void bit_zero(uint8_t *set, size_t num)
{
    memset(set, 0, (num << 3) + 1);
}

#endif /* SRC_UTILS_BIT_H */