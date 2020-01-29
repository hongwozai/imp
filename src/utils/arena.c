#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "arena.h"

void arena_init(Arena *arena, size_t reserved)
{
    assert(arena);

    arena->reserved = reserved;
    arena->first = NULL;
}

void* arena_malloc(Arena *arena, size_t nbytes)
{
    void* ptr;
    struct Hunk* hunk;

    assert(arena);

    /* 如果有内存 */
    for (hunk = arena->first; hunk != NULL; hunk = hunk->next) {
        if (hunk->avail + nbytes <= hunk->limit) {
            ptr = hunk->avail;
            hunk->avail += nbytes;
            return ptr;
        }
    }

    /* newhunk */
    size_t len = sizeof(struct Hunk) + arena->reserved +
        ((nbytes >= arena->reserved) ? nbytes : 0);

    hunk = (struct Hunk*)malloc(len);
    if (!hunk) {
        return NULL;
    }

    hunk->avail = (char*)hunk + sizeof(struct Hunk);
    hunk->limit = (char*)hunk + len;
    hunk->next = arena->first;
    /* link to arena */
    arena->first = hunk;

    ptr = hunk->avail;
    hunk->avail += nbytes;
    return ptr;
}

void arena_dispose(Arena *arena)
{
    struct Hunk* temp;

    while (arena->first) {
        temp = arena->first;
        arena->first = arena->first->next;

        /* free */
        free(temp);
    }

    arena->first = NULL;
}
