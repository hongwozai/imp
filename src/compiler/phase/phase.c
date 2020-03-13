#include <stdbool.h>
#include "phase.h"

static bool iscurrmode(Phase *phase, Node *node)
{
    return node->mode == phase->mode;
}

static bool isvisited(Phase *phase, Node *node)
{
    return node->mode == phase_nextmode(phase);
}

static void setvisited(Phase *phase, Node *node)
{
    /* 这里的访问是包括子节点也访问过 */
    node->mode = (phase->mode == kModeTop) ? kModeBottom : kModeTop;
}

static bool ismiddle(Node *node) { return node->mode == kModeMiddle;}

static void setmiddle(Node *node) { node->mode = kModeMiddle; }

static bool isvisiting(Node *node) { return node->mode == kModeVisit; }
static void setvisiting(Node *node) { node->mode = kModeVisit; }

static void walkalgorithm(Phase *phase, AnalyFunction *func, Node *node,
                          void (*visitfunc)(Phase *phase, AnalyFunction *func, Node *node),
                          void (*walkfunc)(Phase *phase, Node *curr, Node **stack))
{
    Node *first = node;

    /* push stack */
    node->next = NULL;
    setmiddle(node);

    /* loop and fetch */
    while (first) {
        Node *curr = first;

        if (isvisiting(curr)) {
            first = first->next;
            setvisited(phase, curr);

            /* 访问该节点 */
            visitfunc(phase, func, curr);
            continue;
        } else if (isvisited(phase, curr)) {
            first = first->next;
            continue;
        }

        assert(ismiddle(curr));
        setvisiting(curr);

        /* 向栈中压入子节点 */
        walkfunc(phase, curr, &first);
    }
}

static void walkfunc_value(Phase *phase, Node *curr, Node **stack)
{
    /* 利用一个链表，达到顺序的效果 */
    Node *start = NULL, *end = NULL;

    /* find parent ctrl node, right to left push */
    ptrvec_foreach(iter, &curr->ctrls) {
        Node *parent = ptrvec_get(&curr->ctrls, iter);
        if (parent->op == kNodeRegion) {
            continue;
        }

        if (iscurrmode(phase, parent)) {
            setmiddle(parent);

            if (!start) { start = parent; }
            if (end) { end->next = parent; }
            end = parent;
        }
    }

    /* find parent value node, right to left push */
    ptrvec_foreach(iter, &curr->inputs) {
        Node *parent = ptrvec_get(&curr->inputs,
                                  ptrvec_count(&curr->inputs) - iter - 1);

        if (iscurrmode(phase, parent)) {
            setmiddle(parent);

            if (!start) { start = parent; }
            if (end) { end->next = parent; }
            end = parent;
        }
    }

    if (end) {
        end->next = *stack;
        *stack = start;
    }
}

static void visitfunc_value(Phase *phase, AnalyFunction *func, Node *node)
{
    if (phase->visit_value) {
        phase->visit_value(phase, func, node);
    }
}

static void walk_value(Phase *phase, AnalyFunction *func, Node *node)
{
    walkalgorithm(phase, func, node, visitfunc_value, walkfunc_value);
}

static void walkfunc_region(Phase *phase, Node *curr, Node **stack)
{
    /* find parent region, right to left push */
    ptrvec_foreach(iter, &curr->ctrls) {
        Node *parent = ptrvec_get(&curr->ctrls,
                                  ptrvec_count(&curr->ctrls) - iter - 1);

        while (parent) {
            if (parent->op == kNodeRegion) {
                /* push stack */
                parent->next = *stack;
                *stack = parent;
                setmiddle(parent);
                break;
            }

            if (ptrvec_count(&parent->ctrls) == 0) {
                break;
            } else {
                parent = ptrvec_get(&parent->ctrls, 0);
            }
        }
    }
}

static void visitfunc_region(Phase *phase, AnalyFunction *func, Node *node)
{
    if (phase->visit_region) {
        phase->visit_region(phase, func, node);
    }
    walk_value(phase, func, node->attr.node);
}

static void walk_region(Phase *phase, AnalyFunction *func, Node *region)
{
    walkalgorithm(phase, func, region, visitfunc_region, walkfunc_region);
}

void phase_run(Phase *phase, Analy *analy)
{
    /* 遍历函数 */
    list_foreach(pos, analy->funclist.first) {
        AnalyFunction *func = container_of(pos, AnalyFunction, link);

        if (phase->visit_func) {
            phase->visit_func(phase, func);
        }

        if (pos == analy->funclist.first) {
            phase->mode = func->env->curregion->mode;
        }

        /* 直接访问最后region，最终遍历所有region */
        walk_region(phase, func, func->env->curregion);
    }
}

WalkMode phase_nextmode(Phase *phase)
{
    return (phase->mode == kModeTop) ? kModeBottom : kModeTop;
}
