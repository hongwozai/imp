#ifndef SRC_COMPILER_ANALYSIS_H
#define SRC_COMPILER_ANALYSIS_H

#include "graph.h"
#include "symtab.h"
#include "utils/arena.h"
#include "runtime/object.h"

typedef struct AnalyFunction {
    ListLink link;
    Arena  arena;
    /* 当前函数的符号表 */
    SymTab *cursymtab;
    IrGraph graph;
} AnalyFunction;

typedef struct Analy {
    Arena arena;
    List funclist;
    /* 常量表 */
    SymTab consttab;
} Analy;

void    analysis_init(Analy *analy);
void    analysis_destroy(Analy *analy);
IrNode *analysis_analy(Analy *analy, Object *obj);

#endif /* SRC_COMPILER_ANALYSIS_H */