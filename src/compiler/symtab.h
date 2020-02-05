#ifndef SRC_COMPILER_SYMTAB_H
#define SRC_COMPILER_SYMTAB_H

#include <stdbool.h>
#include "graph.h"
#include "runtime/object.h"
#include "utils/arena.h"
#include "utils/list.h"
#include "utils/hashmap.h"

/* string - node */
typedef struct SymTab {
    struct SymTab *prev;
    HashMap map;
} SymTab;

bool    symtab_create(SymTab *symtab, SymTab *prev);
IrNode* symtab_get(SymTab *symtab, Object *str);
void    symtab_set(SymTab *symtab, Arena *arena, Object *str, IrNode *node);
void    symtab_destroy(SymTab *symtab);

IrNode* symtab_nestget(SymTab *symtab, Object *str);

#endif /* SRC_COMPILER_SYMTAB_H */
