#ifndef SRC_COMPILER_IR_H
#define SRC_COMPILER_IR_H

#include "operator.h"
#include "utils/arena.h"
#include "runtime/object.h"

typedef struct IrNode {
    /* 操作符 */
    IrOpcode op;
    bool walkmode;
    /* 输入，顺序为value/effect/control */
    struct IrNode **inputs;
    size_t value_count;
    size_t effect_count;
    size_t control_count;
    /* 属性 */
    IrAttr attr;
} IrNode;

/**
 * constobj
 * 1. obj is str, label obj
 * 2. obj is int, < uint32_t is immediate, > uint64_t is label(generate name)
 * 3. obj is float,
 * 4. is not atom, cons/list/vector/map generate
 */
IrNode* irnode_newconstobj(Arena *arena, IrNode *control, Object *obj);

/**
 * callobj
 * 1 control, 2 effect
 * funcobj, args...
 */
IrNode* irnode_newcallobj(Arena *arena,
                          IrNode *control, IrNode *effect,
                          size_t value_count, IrNode **value_inputs);

/**
 * region
 * inputs[0] -> last value
 * inputs[1] -> last control
 */
IrNode* irnode_newregion(Arena *arena, size_t control_count,
                         IrNode **control);

/**
 * const label
 */
IrNode* irnode_newlabel(Arena *arena, IrNode *control,
                        char *label,
                        char *content, size_t size);

/**
 * const immediate
 */
IrNode* irnode_newimm(Arena *arena, IrNode *control, size_t value);

#endif /* SRC_COMPILER_IR_H */
