#include "graph.h"
#include "operator.h"

IrNode* irnode_newconstobj(Arena *arena, IrNode *control, Object *obj)
{
    IrNode *node = arena_malloc(arena, sizeof(IrNode));
    node->op = kOpConstObj;
    node->value_count = 0;
    node->effect_count = 0;
    node->control_count = 1;
    node->walkmode = true;

    node->inputs = arena_malloc(arena, sizeof(IrNode*));
    node->inputs[0] = control;
    node->attr.objattr.obj = obj;
    return node;
}

IrNode* irnode_newcallobj(Arena *arena,
                          IrNode *control, IrNode *effect,
                          size_t value_count, IrNode **value_inputs)
{
    IrNode *node = arena_malloc(arena, sizeof(IrNode));
    node->op = kOpCallObj;
    node->value_count = value_count;
    node->effect_count = 1;
    node->control_count = 1;
    node->walkmode = true;

    node->inputs =
        arena_malloc(arena, sizeof(IrNode*) *
                     (node->value_count +
                      node->effect_count +
                      node->control_count));

    memcpy(node->inputs, value_inputs, sizeof(IrNode*) * node->value_count);
    node->inputs[node->value_count] = effect;
    node->inputs[node->value_count + node->effect_count] = control;
    return node;
}

IrNode* irnode_newregion(Arena *arena, size_t control_count,
                         IrNode **control_inputs)
{
    IrNode *node = arena_malloc(arena, sizeof(IrNode));
    node->op = kOpRegion;
    node->value_count = 1;
    node->effect_count = 1;
    node->control_count = control_count;
    node->walkmode = true;

    node->inputs =
        arena_malloc(arena, sizeof(IrNode*) *
                     (node->value_count +
                      node->effect_count +
                      node->control_count));

    node->inputs[0] = NULL;
    node->inputs[1] = NULL;
    memcpy(node->inputs + 2,
           control_inputs,
           sizeof(IrNode*) * node->control_count);
    return node;
}
