#ifndef SRC_COMPILER_ANALYSIS_H
#define SRC_COMPILER_ANALYSIS_H

#include "nodes.h"
#include "symtab.h"
#include "utils/arena.h"
#include "runtime/object.h"

typedef struct AnalyEnv {
    /* last exit region */
    Node *curregion;
    SymTab *cursymtab;
} AnalyEnv;

typedef struct AnalyFunction {
    ListLink link;
    Arena  arena;
    AnalyEnv *env;
    /* 当前函数所在的函数，当前函数退出作用域时，parent成为当前函数 */
    struct AnalyFunction *parent;
} AnalyFunction;

typedef struct Analy {
    Arena arena;
    /* 函数表 */
    List funclist;
    /* 当前函数 */
    AnalyFunction *curfunc;
    /* 常量表 */
    SymTab consttab;
} Analy;

void analysis_init(Analy *analy);
void analysis_destroy(Analy *analy);
void analysis_analy(Analy *analy, Object *obj);

#endif /* SRC_COMPILER_ANALYSIS_H */
