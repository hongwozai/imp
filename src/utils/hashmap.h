#ifndef SRC_UTILS_HASHMAP_H
#define SRC_UTILS_HASHMAP_H

#include <stddef.h>
#include <stdbool.h>
#include "list.h"
#include "arena.h"

/* TODO: 改为内核hlist */
typedef struct HashLink {
    ListLink link;
} HashLink;

typedef struct HashMap {
    Arena *arena;
    size_t (*hash)(void *key);
    void*  (*key)(HashLink *link);
    bool   (*equal)(void *key, HashLink *hlink);
    size_t (*extend)(struct HashMap *map);
    /* bucket */
    List *map;
    size_t bucket_size;
    size_t nodenum;
} HashMap;

static inline void hashmap_initlink(HashLink *link)
{
    list_link_init(&link->link);
}

bool hashmap_create(HashMap *map, size_t bucket_size,
                    size_t (*hash)(void *key),
                    void*  (*key)(HashLink *link),
                    bool   (*equal)(void *key, HashLink *hlink),
                    size_t (*extend)(struct HashMap *map)
    );

bool hashmap_arena_create(HashMap *map, Arena *arena, size_t bucket_size,
                          size_t (*hash)(void *key),
                          void*  (*key)(HashLink *link),
                          bool   (*equal)(void *key, HashLink *hlink),
                          size_t (*extend)(struct HashMap *map)
    );

void hashmap_destroy(HashMap *map);

HashLink* hashmap_get(HashMap *map, void *key);
HashLink* hashmap_set(HashMap *map, HashLink *link, bool isupdate);
HashLink* hashmap_del(HashMap *map, void *key);

size_t hashmap_noextend_func(struct HashMap *map);
size_t hashmap_extend2pow_func(struct HashMap *map);

#endif /* SRC_UTILS_HASHMAP_H */
