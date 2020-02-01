#ifndef SRC_RUNTIME_SYMBOL_H
#define SRC_RUNTIME_SYMBOL_H

#include "object.h"
#include "utils/hashmap.h"

/* string -> symbol */
typedef struct SymbolTable {
    HashMap map;
} SymbolTable;

Object* symbol_get(GC *gc);

#endif /* SRC_RUNTIME_SYMBOL_H */