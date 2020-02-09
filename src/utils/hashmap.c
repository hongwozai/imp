#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

size_t hashmap_noextend_func(struct HashMap *map)
{
    return map->bucket_size;
}

size_t hashmap_extend2pow_func(struct HashMap *map)
{
    size_t newsize = map->bucket_size << 1;

    if (newsize < map->bucket_size) {
        return map->bucket_size;
    }
    if (map->nodenum >= map->bucket_size) {
        return newsize;
    }
    return map->bucket_size;
}

bool hashmap_create(HashMap *map, size_t bucket_size,
                    size_t (*hash)(void *key),
                    void*  (*key)(HashLink *link),
                    bool   (*equal)(void *key, HashLink *hlink),
                    size_t (*extend)(struct HashMap *map))
{
    map->map = (List*)malloc(sizeof(List) * bucket_size);
    if (!map->map) {
        return false;
    }
    for (size_t i = 0; i < bucket_size; i++) {
        list_init(&map->map[i]);
    }
    map->bucket_size = bucket_size;
    map->nodenum = 0;
    map->extend = extend;
    map->hash = hash;
    map->equal = equal;
    map->key = key;
    map->arena = NULL;
    return true;
}

bool hashmap_arena_create(HashMap *map, Arena *arena, size_t bucket_size,
                          size_t (*hash)(void *key),
                          void*  (*key)(HashLink *link),
                          bool   (*equal)(void *key, HashLink *hlink),
                          size_t (*extend)(struct HashMap *map))
{
    map->map = (List*)arena_malloc(arena, sizeof(List) * bucket_size);
    if (!map->map) {
        return false;
    }
    for (size_t i = 0; i < bucket_size; i++) {
        list_init(&map->map[i]);
    }
    map->bucket_size = bucket_size;
    map->nodenum = 0;
    map->extend = extend;
    map->hash = hash;
    map->equal = equal;
    map->key = key;
    map->arena = arena;
    return true;
}

void hashmap_destroy(HashMap *map)
{
    if (!map->arena) {
        free(map->map);
    }
    map->map = NULL;
    map->bucket_size = 0;
}

static HashLink *get_internal(HashMap *map, void *key, size_t hash)
{
    list_foreach(temp, map->map[hash].first) {
        HashLink *hlink = container_of(temp, HashLink, link);
        if (map->equal(key, hlink)) {
            return hlink;
        }
    }
    return NULL;
}

HashLink* hashmap_get(HashMap *map, void *key)
{
    size_t hash = map->hash(key) % map->bucket_size;

    return get_internal(map, key, hash);
}

void rehash(HashMap *map, size_t newsize)
{
    List *newmap;
    if (map->arena) {
        newmap = (List*)arena_malloc(map->arena, newsize * sizeof(List));
    } else {
        newmap = (List*)malloc(newsize * sizeof(List));
    }

    for (size_t i = 0; i < map->bucket_size; i++) {
        list_foreach(listlink, map->map[i].first) {
            size_t newhash = map->hash(
                map->key(container_of(listlink, HashLink, link))
                ) % newsize;
            list_push(&newmap[newhash], listlink);
        }
    }

    if (!map->arena) {
        free(map->map);
    }
    map->map = newmap;
    map->bucket_size = newsize;
}

HashLink* hashmap_set(HashMap *map, HashLink *link, bool isupdate)
{
    void *key = map->key(link);
    size_t hash = map->hash(key) % map->bucket_size;
    HashLink *old = get_internal(map, key, hash);

    if (!old) {
        list_push(&map->map[hash], &link->link);
        map->nodenum++;

        /* 新增节点，需要判断是否extend */
        size_t newsize = map->extend(map);
        if (newsize > map->bucket_size) {
            rehash(map, newsize);
        }
        return NULL;
    }
    if (isupdate) {
        *link = *old;
    }
    return old;
}

HashLink* hashmap_del(HashMap *map, void *key)
{
    size_t hash = map->hash(key) % map->bucket_size;
    HashLink *old = get_internal(map, key, hash);

    if (old) {
        list_del(&map->map[hash], &old->link);
        map->nodenum --;
        return old;
    }
    return NULL;
}
