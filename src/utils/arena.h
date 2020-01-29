#ifndef SRC_UTILS_ARENA_H
#define SRC_UTILS_ARENA_H

#include <stddef.h>

struct Hunk {
    struct Hunk* next;
    char* avail;
    char* limit;
};

typedef struct Arena {
    /* 每次申请的大小 */
    size_t reserved;
    /* 实存块链表 */
    struct Hunk *first;
} Arena;

void  arena_init(Arena *arena, size_t reserved);
void* arena_malloc(Arena *arena, size_t nbytes);
void  arena_dispose(Arena *arena);

#endif /* SRC_UTILS_ARENA_H */