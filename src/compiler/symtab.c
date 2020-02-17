#include "runtime/object.h"
#include "symtab.h"

typedef struct HashNode {
    HashLink hlink;
    Object *str;
    Node *irnode;
} HashNode;

/* BKDR */
static size_t hashfunc(void *key)
{
    size_t seed = 131313;
    size_t hash = 0;

    switch (gettype(key)) {
    case kString: {
        StringObject *str = (StringObject*)key;

        for (size_t i = 0; i < str->size; i++) {
            hash = hash * seed + str->buf[i];
        }
        return hash;
    }
    case kFixInt:
        return getvalue(key).fixint;
    case kFixFloat:
        return getvalue(key).fixfloat;
    }
    return hash;
}

static bool equalfunc(void *key, HashLink *hlink)
{
    Object *other = container_of(hlink, HashNode, hlink)->str;
    return equal_object((Object*)key, other);
}

static void *keyfunc(HashLink *link)
{
    HashNode *node = container_of(link, HashNode, hlink);
    return (void*)node->str;
}

bool symtab_create(SymTab *symtab, Arena *arena, SymTab *prev)
{
    symtab->prev = prev;
    if (!hashmap_arena_create(&symtab->map, arena, 32,
                              hashfunc,
                              keyfunc,
                              equalfunc,
                              hashmap_extend2pow_func)) {
        return false;
    }
    return true;
}

Node* symtab_get(SymTab *symtab, Object *str)
{
    HashLink *hlink = hashmap_get(&symtab->map, (void*)str);
    if (!hlink) {
        return NULL;
    }
    return container_of(hlink, HashNode, hlink)->irnode;
}

Node* symtab_nestget(SymTab *symtab, Object *str)
{
    Node *node = NULL;
    for (SymTab *tab = symtab; tab != NULL; tab = tab->prev) {
        node = symtab_get(tab, str);
        if (node) {
            return node;
        }
    }
    return node;
}

void symtab_set(SymTab *symtab, Arena *arena, Object *str, Node *node)
{
    HashNode *hnode = arena_malloc(arena, sizeof(HashNode));
    hashmap_initlink(&hnode->hlink);
    hnode->str = str;
    hnode->irnode = node;
    hashmap_set(&symtab->map, &hnode->hlink, true);
}

void symtab_destroy(SymTab *symtab)
{
    hashmap_destroy(&symtab->map);
}
