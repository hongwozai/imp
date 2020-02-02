#ifndef SRC_COMPILER_IR_H
#define SRC_COMPILER_IR_H

#include "utils/arena.h"
#include "runtime/object.h"
#include "operator.h"

typedef struct IrNodeAttr {
} IrNodeAttr;

typedef struct IrNode IrNode;

struct IrNode {
    IrOperator *op;
    IrNode **values;
    IrNode **controls;
    size_t value_count;
    size_t control_count;
    IrNodeAttr *attr;
};

typedef struct IrGraph {
    IrNode *end;
} IrGraph;

IrNode* irnode_new(Arena *arena, IrOperator *op);

#endif /* SRC_COMPILER_IR_H */