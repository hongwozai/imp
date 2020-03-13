#include <stdio.h>
#include "compiler/nodes.h"
#include "deobj_phase.h"

static void setmode(Phase *phase, Node *node)
{
    node_setmode(node, phase_nextmode(phase));
}

static char *const_getname(Arena *arena)
{
    static size_t const_num = 0;

    /* 64为位足够使用，所以这里不担心其错误码 */
    char buf[64];
    size_t len = snprintf(buf, sizeof(buf), ".const%lu", const_num++);
    return arena_dup(arena, buf, len);
}


static Node* convobj(Phase *phase, AnalyFunction *func,
                     enum ObjectType type, Node *value)
{
    /* <obj: (call gc_vnew type value)> */
    Node *gcnewnode =
        node_newlabel(
            &func->arena,
            arena_dup(&func->arena, "gc_vnew", sizeof("gc_vnew")),
            0,
            0
            );
    setmode(phase, gcnewnode);

    Node *typenode = node_new(&func->arena, kNodeImm);
    setmode(phase, typenode);
    typenode->attr.imm = (size_t)type;

    Node *callnode = node_new(&func->arena, kNodeCall);
    node_addinput(&func->arena, callnode, gcnewnode, false);
    node_addinput(&func->arena, callnode, typenode, false);
    node_addinput(&func->arena, callnode, value, false);
    setmode(phase, callnode);
    return callnode;
}

static Node* deconstobj(Phase *phase, AnalyFunction *func, Node *node)
{
    /* 仅数字判断下大小，创建立即数；其余常量同global，只是不extern */
    if (gettype(node->attr.obj) == kFixInt) {
        /* value */
        Node *temp = node_new(&func->arena, kNodeImm);
        setmode(phase, temp);
        temp->attr.imm = getvalue(node->attr.obj).fixint;

        /* construct value object */
        return convobj(phase, func, kFixInt, temp);
    }

    /* 非整数的值类型 */
    if (gettype(node->attr.obj) < VALUE_TYPE_MAX) {
        /* <value: (load (label name)) */
        Node *label =
            node_newlabel(
                &func->arena,
                const_getname(&func->arena),
                (char*)&getvalue(node->attr.obj),
                sizeof(Value)
                );
        setmode(phase, label);

        Node *load = node_new(&func->arena, kNodeLoad);
        setmode(phase, load);
        return convobj(phase, func, gettype(node->attr.obj), load);
    }

    /* 各种复合类型等 */
    assert(!"not support");
    return NULL;
}

Node *defuncobj(Phase *phase, Arena *arena, Node *funcobj)
{
    /**
     * (load (add funcobj sizeof(type)))
     */
    Node *imm = node_new(arena, kNodeImm);
    setmode(phase, imm);
    imm->attr.imm = sizeof(Object);

    Node *add = node_new(arena, kNodeAdd);
    setmode(phase, add);
    node_addinput(arena, add, funcobj, false);
    node_addinput(arena, add, imm, false);

    Node *load = node_new(arena, kNodeLoad);
    setmode(phase, load);
    node_addinput(arena, load, add, false);

    /**
     * TODO: 这里需要检查第一个参数的类型
     * 如果编译时无法决定，那么需要运行时判断
     */
    return load;
}

Node *deglobalobj(Phase *phase, AnalyFunction *func, Node *node)
{
    /**
     * 目前只支持函数
     */
    Node *label = node_newlabel(&func->arena,
                                str_getcstr(sym_getname(node->attr.obj)),
                                0,
                                0);
    setmode(phase, label);
    return convobj(phase, func, kPrimFunc, label);
}


static void deobj_value(Phase *phase, AnalyFunction *func, Node *node)
{
    switch (node->op) {
    case kNodeConstObj: {
        Node *temp = deconstobj(phase, func, node);
        /* replace node with temp */
        node_vreplace(&func->arena, node, temp);
        break;
    }
    case kNodeGlobalObj: {
        Node *temp = deglobalobj(phase, func, node);
        /* label node with global object name */
        node_vreplace(&func->arena, node, temp);
        break;
    }
    case kNodeCallObj: {
        /* 将函数字段替换为读取函数对象 */
        assert(ptrvec_count(&node->inputs) != 0);

        /* 只第一个输入改为读取的数值即可 */
        node_setop(node, kNodeCall);
        node_replaceinput(&func->arena, node, false, 0,
                          defuncobj(phase, &func->arena,
                                    ptrvec_get(&node->inputs, 0)));
        break;
    }
    default:
        /* skip */
        break;
    }
}

static Phase phase = {
    .visit_func = NULL,
    .visit_region = NULL,
    .visit_value = deobj_value,
};

Phase *deobj_phase = &phase;