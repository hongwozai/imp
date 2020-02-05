#include <assert.h>
#include "phase.h"
#include "runtime/object.h"

static size_t labelid = 0;

static size_t getlabel()
{
    return labelid ++;
}

static IrNode* gen_valueobj(Arena *arena, enum ObjectType)
{
    return;
}

static void lowering_visit_constobj(AnalyFunction *func,
                                    IrNode *node, bool walkmode)
{
    if (walkmode == node->walkmode) { return; }
    node->walkmode = walkmode;

    /* 根据类型生成 */
    switch (gettype(node->attr.objattr.obj)) {
    case kFixInt: {
        break;
    }
    case kFixFloat:
        break;
    case kBool:
    case kChar:
    case kEof:
    case kNil:
    case kString:
        break;
    default:
        panic("bug!");
    }
}

static void lowering_visit_node(AnalyFunction *func,
                                IrNode *node, bool walkmode)
{
    assert(node);

    if (walkmode == node->walkmode) { return; }
    node->walkmode = walkmode;

    switch (node->op) {
    case kOpConstObj: {
        /* print_object(stdout, node->attr.objattr.obj); */
        lowering_visit_constobj(node, walkmode);
        break;
    }
    case kOpCallObj: {
        /* 访问effect链 */
        if (node->inputs[node->value_count]) {
            lowering_visit_node(node->inputs[node->value_count], walkmode);
        }

        for (size_t i = 0; i < node->value_count; i++) {
            lowering_visit_node(node->inputs[i], walkmode);
        }

        break;
    }
    default:
        /* nothing */
        break;
    }
}

static void lowering_visit_region(AnalyFunction *func,
                                  IrNode *region, bool walkmode)
{
    assert(region);
    if (walkmode == region->walkmode) { return; }
    region->walkmode = walkmode;

    for (size_t i = 0; i < region->control_count; i++) {
        lowering_visit_region(region->inputs[i + 2], walkmode);
    }

    /* 访问本region */
    if (region->inputs[0]) {
        lowering_visit_node(region->inputs[0], walkmode);
    }
    if (region->inputs[1]) {
        lowering_visit_node(region->inputs[1], walkmode);
    }
}

void phase_lowering(AnalyFunction *func)
{
    lowering_visit_region(func,
                          func->env->curregion,
                          !func->env->curregion->walkmode);
}

void phase_run(Analy *analy)
{
    list_foreach(pos, analy->funclist.first) {
        AnalyFunction *func = container_of(pos, AnalyFunction, link);

        /* phase run */
        phase_lowering(func);
    }
}
