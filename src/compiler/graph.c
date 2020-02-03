#include "operator.h"
#include "graph.h"

static size_t irnode_globalid = 0;

static size_t getid()
{
    return irnode_globalid ++;
}

IrNode* irnode_new(Arena *arena, int openum,
                   size_t value_count, size_t control_count, IrNode **inputs,
                   void *attr)
{
    IrNode *node = arena_malloc(arena, sizeof(IrNode));
    node->id = getid();
    node->op = &irallop[openum];
    node->inputs = inputs;
    node->value_count = value_count;
    node->control_count = control_count;
    if (irallop[openum].have_attr) {
        node->attr = attr;
    }
    return node;
}