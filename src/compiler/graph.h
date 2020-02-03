#ifndef SRC_COMPILER_IR_H
#define SRC_COMPILER_IR_H

#include "operator.h"
#include "utils/arena.h"
#include "runtime/object.h"

typedef struct IrNode IrNode;

enum {
    kHintUnknow = -1,
};

struct IrNode {
    size_t id;
    /* 操作符 */
    IrOperator *op;
    /* 该节点的输入，先value后control */
    IrNode **inputs;
    size_t value_count;
    size_t control_count;
    /* 属性 */
    void *attr;
    /* 当为值节点时，其值的类型 */
    int typehint;
};

IrNode* irnode_new(Arena *arena, int openum,
                   size_t value_count, size_t control_count, IrNode **inputs,
                   void *attr);
bool irnode_isdead(IrNode *node);

#endif /* SRC_COMPILER_IR_H */
