#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "utils/hashmap.h"

typedef struct Node {
    HashLink hlink;
    char *name;
    long value;
} Node;

static size_t bkdrhash(void *key)
{
    char *str = (char*)key;
    size_t seed = 131313;
    size_t hash = 0;
    while(*str) {
        hash = hash * seed + (*str++);
    }
    return hash;
}

static bool equal(void *key, HashLink *hlink)
{
    char *str = (char*)key;
    return strcmp(str, ((Node*)hlink)->name) == 0;
}

static void* key(HashLink *link)
{
    Node *node = (Node*)link;
    return node->name;
}

int main(int argc, char *argv[])
{
    HashMap map;
    Node node[] = {
        {
            .hlink = { INIT_LINK },
            .name = "n1",
            .value = 1,
        },
        {
            .hlink = { INIT_LINK },
            .name = "n2",
            .value = 2,
        },
        {
            .hlink = { INIT_LINK },
            .name = "n3",
            .value = 3,
        }
    };
    hashmap_create(&map, 512, bkdrhash, key, equal, hashmap_noextend_func);
    if (NULL != hashmap_set(&map, &node[0].hlink, true)) {
        exit(-1);
    }
    if (hashmap_get(&map, (void*)"n1") != (HashLink*)&node[0]) {
        exit(-2);
    }
    if (hashmap_get(&map, (void*)"n2") != NULL) {
        exit(-3);
    }
    if (NULL != hashmap_set(&map, &node[1].hlink, true)) {
        exit(-4);
    }
    if (hashmap_get(&map, (void*)"n2") == NULL) {
        exit(-3);
    }
    hashmap_destroy(&map);
    return 0;
}