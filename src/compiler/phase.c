#include <assert.h>

#include "utils/arena.h"
#include "runtime/object.h"
#include "nodes.h"
#include "phase.h"

static size_t g_const_num = 0;

static size_t get_const_num()
{
    return ++g_const_num;
}

static char *get_constname(Arena *arena)
{
    char buf[64] = {0};
    size_t num = get_const_num();

    size_t i = 0;
    for (size_t i = 0; num > 0; i++) {
        buf[i] = (num % 10) + '0';
        num /= 10;
        i++;
    }

    char *name = arena_malloc(arena, i + sizeof(".Const"));

    size_t iter;
    memcpy(name, ".Const", 6);
    for (iter = 6; i > 0; i--, iter++) {
        name[iter] = buf[i];
    }
    name[iter] = 0;
    return name;
}

static Node* construct_value_object(Phase *phase, AnalyFunction *analyfunc,
                                    enum ObjectType type, Node *value)
{
    /**
     * <obj: (call gc_vnew type value)>
     */
    Node *callnode = node_new(&analyfunc->arena, kNodeCall);
    Node *gcnewnode = node_new(&analyfunc->arena, kNodeLabel);
    Node *typenode = node_new(&analyfunc->arena, kNodeImm);

    gcnewnode->attr.label.name = arena_dup(&analyfunc->arena,
                                           "gc_vnew", sizeof("gc_vnew"));
    gcnewnode->attr.label.data = 0;
    gcnewnode->attr.label.datalen = 0;

    typenode->attr.imm = (size_t)type;

    node_addinput(&analyfunc->arena, callnode, gcnewnode, false);
    node_addinput(&analyfunc->arena, callnode, typenode, false);
    node_addinput(&analyfunc->arena, callnode, value, false);
    return callnode;
}

static Node* construct_labelvalue_object(Phase *phase, AnalyFunction *analyfunc,
                                         enum ObjectType type,
                                         char *name,
                                         char *data,
                                         size_t datalen)
{
    /**
     * <value: (load (label name))
     * (call gc_vnew type value)
     */
    Node *label = node_new(&analyfunc->arena, kNodeLabel);
    label->attr.label.name = arena_dup(&analyfunc->arena, name, strlen(name));
    label->attr.label.data = arena_dup(&analyfunc->arena, data, datalen);
    label->attr.label.datalen = datalen;

    Node *load = node_new(&analyfunc->arena, kNodeLoad);
    node_addinput(&analyfunc->arena, load, label, false);

    return construct_value_object(phase, analyfunc, type, load);
}

/* =========================================================== */
static Node* deobj_constobj(Phase *phase, AnalyFunction *analyfunc, Node *node)
{
    Node *temp;

    /* 仅数字判断下大小，创建立即数；其余常量同global，只是不extern */
    if (gettype(node->attr.obj) == kFixInt) {
        temp = node_new(&analyfunc->arena, kNodeImm);
        temp->attr.imm = (size_t)getvalue(node->attr.obj).fixint;
        temp = construct_value_object(phase, analyfunc, kFixInt, temp);
    } else if (gettype(node->attr.obj) < VALUE_TYPE_MAX) {
        Value v = getvalue(node->attr.obj);
        temp = construct_labelvalue_object(phase, analyfunc,
                                           gettype(node->attr.obj),
                                           get_constname(&analyfunc->arena),
                                           (char*)&v,
                                           sizeof(Value));
    } else {
        /* 各种复合类型等 */
        assert(!"not support");
    }
    return temp;
}

static void deobj_region(Phase *phase, AnalyFunction *analyfunc, Node *node) {}

static void deobj_value(Phase *phase, AnalyFunction *analyfunc, Node *node)
{
    Node *temp;

    switch (node->op) {
    case kNodeConstObj: {
        temp = deobj_constobj(phase, analyfunc, node);
        /* replace node with temp */
        node_vreplace(&analyfunc->arena, node, temp);
        break;
    }

    case kNodeGlobalObj: {
        /* label */
        temp = node_new(&analyfunc->arena, kNodeLabel);
        temp->attr.label.name = arena_dup(&analyfunc->arena,
                                          str_getcstr(sym_getname(node->attr.obj)),
                                          str_getsize(sym_getname(node->attr.obj)) + 1);
        temp->attr.label.data = 0;
        temp->attr.label.datalen = 0;

        temp = construct_value_object(phase, analyfunc, kPrimFunc, temp);
        /* label node with global object name */
        node_vreplace(&analyfunc->arena, node, temp);
        break;
    }

    case kNodeCallObj: {
        /* 将函数字段替换为读取函数对象 */
        assert(ptrvec_count(&node->inputs) != 0);

        Node *funcobj = ptrvec_get(&node->inputs, 0);
        Node *imm = node_new(&analyfunc->arena, kNodeImm);
        imm->attr.imm = sizeof(Value);

        Node *temp = node_new(&analyfunc->arena, kNodeLoad);
        node_addinput(&analyfunc->arena, temp, funcobj, false);
        node_addinput(&analyfunc->arena, temp, imm, false);

        /**
         * TODO: 这里需要检查第一个参数的类型
         * 如果编译时无法决定，那么需要运行时判断
         */

        /* 只第一个输入改为读取的数值即可 */
        node->op = kNodeCall;
        node_replaceinput(&analyfunc->arena, node, false, 0, temp);
        break;
    }

    default:
        /* skip */
        break;
    }
}

Phase deobj_phase = {
    .visit_func = NULL,
    .visit_region = deobj_region,
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
    .visit_func = NULL,
    .visit_region = print_value,
    .visit_value = print_region,
};
/* =========================================================== */

static void phase_visit_value(Phase *phase, AnalyFunction *analyfunc,
                             Node *node, WalkMode mode)
{
    if (node->mode != mode) { return; }
    node->mode = (node->mode == kModeTop) ? kModeBottom : kModeTop;

    if (phase->visit_value) {
        phase->visit_value(phase, analyfunc, node);
    }
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
    if (phase->visit_region) {
        phase->visit_region(phase, analyfunc, region);
    }

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

void phase_walk(Phase *phase, Analy *analy)
{
    list_foreach(pos, analy->funclist.first) {
        AnalyFunction *analyfunc = container_of(pos, AnalyFunction, link);

        if (phase->visit_func) {
            phase->visit_func(phase, analyfunc);
        }
        /* 直接访问最后region，最终遍历所有region */
        phase_walk_region(phase, analyfunc,
                          analyfunc->env->curregion,
                          analyfunc->env->curregion->mode);
    }
}

void phase_run(Analy *analy)
{
    phase_walk(&deobj_phase, analy);
    phase_walk(&print_phase, analy);
}
