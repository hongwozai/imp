#ifndef SRC_COMPILER_PTRVEC_H
#define SRC_COMPILER_PTRVEC_H

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include "utils/arena.h"

typedef struct PtrVec {
    void **elems;
    size_t count;
    size_t capacity;
} PtrVec;

#define ptrvec_foreach(iter, ptrvec)                        \
    for (size_t iter = 0; iter < (ptrvec)->count; ++iter)

static inline void ptrvec_init(Arena *arena, PtrVec *vec, size_t count)
{
    if (count != 0) {
        vec->elems = arena_malloc(arena, count * sizeof(void*));
        vec->count = 0;
        vec->capacity = count;
    } else {
        vec->elems = NULL;
        vec->count = 0;
        vec->capacity = 0;
    }
}

static inline size_t ptrvec_add(Arena *arena, PtrVec *vec, void *elem)
{
    if (vec->capacity <= vec->count) {
        /* 初始值为1 */
        size_t newcap = (vec->capacity == 0 ? 1 : vec->capacity) << 1;
        if (vec->capacity != 0 && newcap <= vec->capacity) {
            return -1;
        }
        void **newbuf = arena_malloc(arena, sizeof(void*) * newcap);
        memcpy(newbuf, vec->elems, sizeof(void*) * vec->count);
        vec->elems = newbuf;
        vec->capacity = newcap;
    }
    vec->elems[vec->count ++] = elem;
    return vec->count - 1;
}

static inline size_t ptrvec_count(PtrVec *vec)
{
    return vec->count;
}

static inline bool ptrvec_empty(PtrVec *vec)
{
    return vec->count == 0;
}

static inline void* ptrvec_get(PtrVec *vec, size_t index)
{
    assert(index < vec->count);
    return vec->elems[index];
}

static inline void* ptrvec_set(PtrVec *vec, size_t index, void *elem)
{
    void *old = vec->elems[index];
    vec->elems[index] = elem;
    return old;
}

#endif /* SRC_COMPILER_PTRVEC_H */