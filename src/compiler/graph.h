#ifndef SRC_COMPILER_IR_H
#define SRC_COMPILER_IR_H

#include "runtime/object.h"
#include "operator.h"

typedef struct IrNodeAttr {
} IrNodeAttr;

typedef struct IrNode IrNode;

struct IrNode {
    IrOperator *op;
    IrNode **inputs;
    IrNode **control_inputs;
    size_t value_input_count;
    size_t control_input_count;
    IrNodeAttr *attr;
};

typedef struct IrGraph {
    IrNode *end;
} IrGraph;

#endif /* SRC_COMPILER_IR_H */