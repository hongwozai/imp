#include <assert.h>
#include <stdarg.h>
#include "insts.h"
#include "instgen.h"
#include "instgen_internal.h"
#include "compiler/nodes.h"

/**
 * (Load (Add reg imm))  -> mov $imm(vreg), vreg
 * (Load reg)            -> mov (vreg), vreg
 * (Call (Label) ...)    -> call (label); mov $rax, vreg
 * (Call reg ...)        -> args...; call reg; mov $rax, vreg
 * (Imm num)             -> mov $num, vreg
 * (Label name)          -> lea (name), vreg
 * (Add reg imm)         -> add $imm, vreg
 * (Add reg1 reg2)       -> add vreg1, vreg2
 */

static void genload(ModuleFunc *func, Node *node, Node **stack);
static void gencall(ModuleFunc *func, Node *node, Node **stack);
static void genlabel(ModuleFunc *func, Node *node, Node **stack);
static void genadd(ModuleFunc *func, Node *node, Node **stack);
static void genimm(ModuleFunc *func, Node *node, Node **stack);
static void emit(ModuleFunc *func, Block *block, Node *node);

static InstGenOp ops[] = {
    [kNodeLoad]  = { .gen = genload,  .emit = emit },
    [kNodeAdd]   = { .gen = genadd,   .emit = emit },
    [kNodeCall]  = { .gen = gencall,  .emit = emit },
    [kNodeLabel] = { .gen = genlabel, .emit = emit },
    [kNodeImm]   = { .gen = genimm,   .emit = emit }
};

InstGenOp *instgenop_x64 = ops;

static Inst *newinst3v(Arena *arena, const char *desc,
                       VirtualReg op1, VirtualReg op2, VirtualReg dst,
                       ...)
{
    va_list ap;
    char buf[512];

    va_start(ap, dst);
    vsnprintf(buf, 512, desc, ap);
    va_end(ap);

    return inst_newv(arena, buf, op1, op2, dst);
}

static InstGenData* markrule(ModuleFunc *func, Node *node)
{
    InstGenData *data = arena_malloc(&func->arena, sizeof(InstGenData));
    data->type = kInstDataRule;
    data->vreg = vreg_alloc(func);
    list_init(&data->insts);
    data->list = NULL;

    node->data = data;
    return data;
}

static void markused(ModuleFunc *func, Node *node)
{
    InstGenData *data = arena_malloc(&func->arena, sizeof(InstGenData));
    data->type = kInstDataUsed;
    data->vreg = vreg_unused();
    list_init(&data->insts);
    data->list = NULL;

    node->data = data;
}

static void genload(ModuleFunc *func, Node *node, Node **stack)
{
    assert(ptrvec_count(&node->inputs) != 0);

    InstGenData *data = markrule(func, node);
    Node *child = ptrvec_get(&node->inputs, 0);

    /* bingo! (Load (Add reg imm)) */
    if (child->op == kNodeAdd) {
        /* (Add reg imm) or (Add imm reg) */
        assert(ptrvec_count(&child->inputs) == 2);

        Node *left = ptrvec_get(&child->inputs, 0);
        Node *right = ptrvec_get(&child->inputs, 1);
        if (left->op == kNodeImm || right->op == kNodeImm) {
            Node *imm = (left->op == kNodeImm) ? left : right;
            Node *reg = (left->op == kNodeImm) ? right : left;

            Inst *inst = newinst3v(&func->arena, "mov $%ld(%%1), %%r",
                                   vreg_placeholder(), vreg_unused(),
                                   data->vreg,
                                   imm->attr.imm);
            list_append(&data->insts, &inst->link);

            markused(func, child);
            markused(func, imm);

            /* push stack */
            if (!instgen_isqueue(reg)) {
                reg->next = *stack;
                *stack = reg;
            }
            data->list = nodelist_new(&func->arena, reg, data->list);
            return;
        }
    }

    /* bingo! (Load reg) */
    Inst *inst = newinst3v(&func->arena, "mov (%%1), %%r",
                           vreg_placeholder(), vreg_unused(),
                           data->vreg);
    list_append(&data->insts, &inst->link);

    /* push */
    data->list = nodelist_new(&func->arena, child, data->list);
    if (!instgen_isqueue(child)) {
        child->next = *stack;
        *stack = child;
    }
}

/**
 * 返回是否需要添加该参数
 */
static bool matchargs(ModuleFunc *func, Node *node, size_t iter,
                      InstGenData *data)
{
    const char *argreg;

    switch (iter) {
    case 1: argreg = "%%rdi"; break;
    case 2: argreg = "%%rsi"; break;
    case 3: argreg = "%%rdx"; break;
    case 4: argreg = "%%rcx"; break;
    case 5: argreg = "%%r8"; break;
    case 6: argreg = "%%r9"; break;
    }

    /* TODO: 处理参数 */
    switch (node->op) {
    case kNodeImm: {
        /* INST0(func, data, "mov $%ld, %s", node->attr.imm, argreg); */
        Inst *inst = newinst3v(&func->arena,
                               "mov $%ld, %s",
                               vreg_unused(), vreg_unused(), vreg_unused(),
                               node->attr.imm, argreg);
        markused(func, node);
        list_append(&data->insts, &inst->link);
        return false;
    }
    case kNodeLabel: {
        Inst *inst = newinst3v(&func->arena,
                               "lea (%s), %s",
                               vreg_unused(), vreg_unused(), vreg_unused(),
                               node->attr.label.name, argreg);
        markused(func, node);
        list_append(&data->insts, &inst->link);
        return false;
    }
    default: {
        /* Inst *inst = newinst3v(&func->arena, */
        /*                        "lea (%s), %s", */
        /*                        vreg_unused(), vreg_unused(), vreg_unused(), */
        /*                        node->attr.label.name, argreg); */
        /* markused(func, node); */
        /* list_append(&data->insts, &inst->link); */
        return true;
    }
    }
}

static void gencall(ModuleFunc *func, Node *node, Node **stack)
{
    Node *funcnode = NULL;
    InstGenData *data = markrule(func, node);

    ptrvec_foreach(iter, &node->inputs) {
        Node *parent = ptrvec_get(&node->inputs, iter);

        /* 函数地址 */
        if (iter == 0) {
            funcnode = parent;

            if (parent->op == kNodeLabel) {
                /* label dont push stack */
                continue;
            }
        }

        /* args */
        if (matchargs(func, parent, iter, data)) {
            /* push stack */
            parent->next = *stack;
            *stack = parent;
        }
    }

    /* (Call (Label) ...) */
    if (funcnode->op == kNodeLabel) {
        Inst *inst = newinst3v(&func->arena, "call (%s)",
                               vreg_unused(), vreg_unused(), vreg_unused(),
                               funcnode->attr.label.name);
        list_append(&data->insts, &inst->link);
        markused(func, funcnode);
        return;
    }

    Inst *inst = newinst3v(&func->arena, "call %%1",
                           data->vreg, vreg_unused(), vreg_unused());
    list_append(&data->insts, &inst->link);
}

static void genadd(ModuleFunc *func, Node *node, Node **stack)
{
    assert(ptrvec_count(&node->inputs) == 2);

    /* child */
    Node *left = ptrvec_get(&node->inputs, 0);
    Node *right = ptrvec_get(&node->inputs, 1);

    /* insert instrutions */
    InstGenData *data = markrule(func, node);
    Inst *inst = newinst3v(&func->arena, "lea (%%1, %%2, 1), %%r",
                           vreg_placeholder(), vreg_placeholder(), data->vreg);
    list_append(&data->insts, &inst->link);

    /* placeholder */
    data->list = nodelist_new(&func->arena, left, data->list);
    data->list = nodelist_new(&func->arena, right, data->list);

    /* push stack */
    if (!instgen_isqueue(right)) {
        right->next = *stack;
        *stack = right;
    }
    if (!instgen_isqueue(left)) {
        left->next = *stack;
        *stack = left;
    }
}

static void genlabel(ModuleFunc *func, Node *node, Node **stack)
{
    InstGenData *data = markrule(func, node);
    Inst *inst = newinst3v(&func->arena,
                           "lea (%s), %%r",
                           vreg_unused(), vreg_unused(), data->vreg,
                           node->attr.label.name);
    list_append(&data->insts, &inst->link);
}

static void genimm(ModuleFunc *func, Node *node, Node **stack)
{
    InstGenData *data = markrule(func, node);
    Inst *inst = newinst3v(&func->arena,
                           "mov $%ld, %%r",
                           vreg_unused(), vreg_unused(), data->vreg,
                           node->attr.imm);
    list_append(&data->insts, &inst->link);
}

static void emit(ModuleFunc *func, Block *block, Node *node)
{
    InstGenData *data = node->data;
    assert(data->type == kInstDataRule);

    /* for debug */
    /* node_dprint(stdout, node); */
    /* fprintf(stdout, "\n"); */

    list_safe_foreach(pos, iter, data->insts.first) {
        Inst *inst = container_of(pos, Inst, link);

        /* 1. 完善之前的指令 */
        for (int i = 0; i < 3; i++) {
            if (vreg_isplaceholder(inst->u.vreg[i])) {
                NodeList *first = data->list;
                InstGenData *childdata = first->node->data;

                inst->u.vreg[i] = childdata->vreg;
                /* pop */
                data->list = data->list->next;
            }
        }

        /* 2. 插入block */
        list_append(&block->insts, &inst->link);

        /* debug */
        inst_dprint(stdout, inst);
    }
}
