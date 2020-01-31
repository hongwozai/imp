#ifndef SRC_UTILS_BIT_H
#define SRC_UTILS_BIT_H

#include <stdint.h>

static inline void bit_set(uint8_t *set, size_t index)
{
    set[index >> 8] |= (1 << (index & 7));
}

static inline int bit_check(uint8_t *set, size_t index)
{
    return set[index >> 8] & (1 << (index & 7));
}

static inline void bit_clear(uint8_t *set, size_t index)
{
    set[index >> 8] &= ~(1 << (index & 7));
}

#endif /* SRC_UTILS_BIT_H */