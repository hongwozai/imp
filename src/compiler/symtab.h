#ifndef SRC_COMPILER_SYMTAB_H
#define SRC_COMPILER_SYMTAB_H

#include <stdbool.h>
#include "nodes.h"
#include "runtime/object.h"
#include "utils/arena.h"
#include "utils/list.h"
#include "utils/hashmap.h"

typedef struct SymNode {
    HashLink hlink;
    Object *str;
    Node *irnode;
    bool isfunc;
    void *any;
} SymNode;

/* string - node */
typedef struct SymTab {
    struct SymTab *prev;
    HashMap map;
} SymTab;

bool    symtab_create(SymTab *symtab, Arena *arena, SymTab *prev);
Node*   symtab_get(SymTab *symtab, Object *str);
void    symtab_set(SymTab *symtab, Arena *arena, Object *str, Node *node);
void    symtab_destroy(SymTab *symtab);

Node*   symtab_nestget(SymTab *symtab, Object *str);

#endif /* SRC_COMPILER_SYMTAB_H */
