#include <stdio.h>
#include "utils/arena.h"

int main(int argc, char *argv[])
{
    Arena arena;
    void *ptr;
    arena_init(&arena, 1024);
    for (int i = 0; i < 100; i++) {
        ptr = arena_malloc(&arena, 100);
    }
    arena_dispose(&arena);
    return 0;
}