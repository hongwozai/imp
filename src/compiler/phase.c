#include "utils/arena.h"
#include "nodes.h"
#include "phase.h"

/* =========================================================== */

static void deobj_region(Phase *phase, AnalyFunction *analyfunc, Node *node)
{
    
}

static void deobj_value(Phase *phase, AnalyFunction *analyfunc, Node *node)
{
    
}

Phase deobj_phase = {
    .visit_region = deobj_value,
    .visit_value = deobj_value,
};
/* =========================================================== */
static void print_value(Phase *phase, AnalyFunction *analyfunc, Node *node)
{
    node_dprint(stderr, node);
    fprintf(stderr, "\n");
}

static void print_region(Phase *phase, AnalyFunction *analyfunc, Node *node)
{
    node_dprint(stderr, node);
    fprintf(stderr, "\n");
}

Phase print_phase = {
    .visit_region = print_value,
    .visit_value = print_region,
};
/* =========================================================== */

static void phase_visit_value(Phase *phase, AnalyFunction *analyfunc,
                             Node *node, WalkMode mode)
{
    if (node->mode != mode) { return; }
    node->mode = (node->mode == kModeTop) ? kModeBottom : kModeTop;

    phase->visit_value(phase, analyfunc, node);
}

static void phase_walk_value(Phase *phase, AnalyFunction *analyfunc,
                             Node *node, WalkMode mode)
{
    /* 控制依赖 */
    ptrvec_foreach(iter, &node->ctrls) {
        Node *parent = ptrvec_get(&node->ctrls, iter);
        if (parent->op != kNodeRegion) {
            phase_walk_value(phase, analyfunc, parent, mode);
        }
    }

    /* 数据依赖 */
    ptrvec_foreach(iter, &node->inputs) {
        Node *input = ptrvec_get(&node->inputs, iter);
        phase_walk_value(phase, analyfunc, input, mode);
    }

    phase_visit_value(phase, analyfunc, node, mode);
}

static void phase_visit_region(Phase *phase, AnalyFunction *analyfunc,
                               Node *region, WalkMode mode)
{
    /* 设置遍历过的标记 */
    if (region->mode != mode) { return; }
    region->mode = (region->mode == kModeTop) ? kModeBottom : kModeTop;

    /* 访问当前region */
    phase->visit_region(phase, analyfunc, region);

    /* 访问所有region的node */
    phase_walk_value(phase, analyfunc, region->attr.node, mode);
}

static void phase_walk_region(Phase *phase, AnalyFunction *analyfunc,
                              Node *region, WalkMode mode)
{
    /* 遍历region */
    ptrvec_foreach(iter, &region->ctrls) {
        Node *n = ptrvec_get(&region->ctrls, iter);
        while (n) {
            if (n->op == kNodeRegion) {
                phase_walk_region(phase, analyfunc, n, mode);
                break;
            }
            n = ptrvec_get(&region->ctrls, 0);
        }
    }

    /* 访问 */
    phase_visit_region(phase, analyfunc, region, mode);

}

static void phase_walk(Phase *phase, Analy *analy)
{
    list_foreach(pos, analy->funclist.first) {
        AnalyFunction *analyfunc = container_of(pos, AnalyFunction, link);
        /* 直接访问最后region，最终遍历所有region */
        phase_walk_region(phase, analyfunc,
                          analyfunc->env->curregion,
                          analyfunc->env->curregion->mode);
    }
}

void phase_run(Analy *analy)
{
    phase_walk(&print_phase, analy);
}
