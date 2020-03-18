#include <assert.h>
#include <stdarg.h>
#include "insts.h"
#include "instgen.h"
#include "instgen_internal.h"
#include "compiler/nodes.h"

#define EMIT(arena, data, fmt, args...)                 \
    list_append(&data->insts,                           \
                &newinst3v(arena, fmt, ##args)->link)

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

static void nodelist_append(Arena *arena, List *list, Node *node)
{
    NodeList *first = arena_malloc(arena, sizeof(NodeList));
    list_link_init(&first->link);
    first->node = node;

    list_append(list, &first->link);
}

static Node *nodelist_pop(List *list)
{
    NodeList *temp = container_of(list_pop(list), NodeList, link);
    return temp->node;
}

static InstGenData* markrule(ModuleFunc *func, Node *node, bool isalloc)
{
    InstGenData *data = arena_malloc(&func->arena, sizeof(InstGenData));
    data->type = kInstDataRule;
    list_init(&data->insts);
    list_init(&data->nodelist);

    data->vreg = isalloc ? vreg_alloc(func) : vreg_unused();

    node->data = data;
    return data;
}

static void markused(ModuleFunc *func, Node *node)
{
    InstGenData *data = arena_malloc(&func->arena, sizeof(InstGenData));
    data->type = kInstDataUsed;
    data->vreg = vreg_unused();
    list_init(&data->insts);
    list_init(&data->nodelist);

    node->data = data;
}

/**
 * 该函数使用场景：
 * 在判断子节点是否可以与当前节点为一个覆盖(无人处理的节点)
 */
static bool isneedhandle(Node *node)
{
    assert(node->data
           ? ((InstGenData*)node->data)->type != kInstDataUsed
           : true);
    return
        !instgen_isqueue(node) &&
        !node->data &&
        node->uses.size == 1;
}

static void genload(ModuleFunc *func, Node *node, Node **stack)
{
    assert(ptrvec_count(&node->inputs) == 1);

    InstGenData *data = markrule(func, node, true);
    Node *child = ptrvec_get(&node->inputs, 0);

    /* bingo! (Load (Add reg imm)) */
    if (child->op == kNodeAdd && isneedhandle(child)) {
        /* (Add reg imm) or (Add imm reg) */
        assert(ptrvec_count(&child->inputs) == 2);

        Node *left = ptrvec_get(&child->inputs, 0);
        Node *right = ptrvec_get(&child->inputs, 1);

        /**
         * 继续进行树覆盖时，必须保证接下来的节点可处理
         * 不可处理的节点一定是在队列中已经化为了其他节点的处理对象
         */
        if ((left->op == kNodeImm && isneedhandle(left)) ||
            (right->op == kNodeImm && isneedhandle(left))) {

            Node *imm = (left->op == kNodeImm) ? left : right;
            Node *reg = (left->op == kNodeImm) ? right : left;

            EMIT(&func->arena, data, "mov %ld(%%1), %%r",
                 vreg_placeholder(), vreg_unused(), data->vreg,
                 imm->attr.imm);
            nodelist_append(&func->arena, &data->nodelist, reg);

            markused(func, child);
            markused(func, imm);

            /* push stack */
            if (isneedhandle(reg)) {
                reg->next = *stack;
                *stack = reg;
            }
            return;
        }
    }

    /* bingo! (Load reg) */
    EMIT(&func->arena, data, "mov (%%1), %%r",
         vreg_placeholder(), vreg_unused(), data->vreg);

    /* push */
    nodelist_append(&func->arena, &data->nodelist,  child);

    if (isneedhandle(child)) {
        child->next = *stack;
        *stack = child;
    }
}

/**
 * 返回是否需要添加该参数
 * 该函数必须以相反的顺序调用
 */
static bool matchargs(ModuleFunc *func, Node *node, size_t iter,
                      InstGenData *data)
{
    const char *argreg = NULL;

    switch (iter) {
    case 1: argreg = "%%rdi"; break;
    case 2: argreg = "%%rsi"; break;
    case 3: argreg = "%%rdx"; break;
    case 4: argreg = "%%rcx"; break;
    case 5: argreg = "%%r8"; break;
    case 6: argreg = "%%r9"; break;
    }

    if (iter <= 6) {
        switch (node->op) {
        case kNodeImm: {
            EMIT(&func->arena, data, "mov $%ld, %s",
                 vreg_unused(), vreg_unused(), vreg_unused(),
                 node->attr.imm, argreg);
            markused(func, node);
            return false;
        }
        case kNodeLabel: {
            EMIT(&func->arena, data, "lea (%s), %s",
                 vreg_unused(), vreg_unused(), vreg_unused(),
                 node->attr.label.name, argreg);
            markused(func, node);
            return false;
        }
        default: {
            EMIT(&func->arena, data, "mov %%1, %s",
                 vreg_placeholder(), vreg_unused(), vreg_unused(),
                 argreg);

            nodelist_append(&func->arena, &data->nodelist,  node);
            return true;
        }
        }
    }

    if (node->op == kNodeImm) {
        EMIT(&func->arena, data, "push %ld",
             vreg_unused(), vreg_unused(), vreg_unused(),
             node->attr.imm);
        return false;
    }

    EMIT(&func->arena, data, "push %%1",
         vreg_placeholder(), vreg_unused(), vreg_unused(),
         argreg);

    nodelist_append(&func->arena, &data->nodelist,  node);
    return true;
}

static void gencall(ModuleFunc *func, Node *node, Node **stack)
{
    Node *funcnode = NULL;
    InstGenData *data = markrule(func, node, node->uses.size != 0);

    ptrvec_foreach(iter, &node->inputs) {
        /* 反向匹配参数 */
        size_t argindex = ptrvec_count(&node->inputs) - iter - 1;
        Node *parent = ptrvec_get(&node->inputs, argindex);

        /* 函数地址 */
        if (argindex == 0) {
            funcnode = parent;

            if (parent->op == kNodeLabel && isneedhandle(parent)) {
                /* label dont push stack */
                continue;
            }
        }

        /* 生成参数相关语句 */
        if (argindex == 0 || matchargs(func, parent, argindex, data)) {
            /* push stack */
            if (isneedhandle(parent)) {
                parent->next = *stack;
                *stack = parent;
            }
        }
    }

    if (funcnode->op == kNodeLabel) {
        /* (Call (Label) ...) */
        EMIT(&func->arena, data,
             "call (%s)",
             vreg_unused(), vreg_unused(), vreg_unused(),
             funcnode->attr.label.name);
        markused(func, funcnode);
    } else {
        /* (Call vreg ...) */
        EMIT(&func->arena, data,
             "call *%%1",
             vreg_placeholder(), vreg_unused(), vreg_unused());

        nodelist_append(&func->arena, &data->nodelist,  funcnode);
    }

    /* 当该调用无人使用时，不申请结果寄存器 */
    if (node->uses.size != 0) {
        EMIT(&func->arena, data,
             "mov %s, %%r",
             vreg_unused(), vreg_unused(), data->vreg,
             "%%rax");
    }
}

static void genadd(ModuleFunc *func, Node *node, Node **stack)
{
    assert(ptrvec_count(&node->inputs) == 2);

    /* child */
    Node *left = ptrvec_get(&node->inputs, 0);
    Node *right = ptrvec_get(&node->inputs, 1);

    /* insert instrutions */
    InstGenData *data = markrule(func, node, true);

    /* emit */
    EMIT(&func->arena, data, "lea (%%1, %%2, 1), %%r",
         vreg_placeholder(), vreg_placeholder(), data->vreg);
    nodelist_append(&func->arena, &data->nodelist,  left);
    nodelist_append(&func->arena, &data->nodelist,  right);

    /* push stack */
    if (isneedhandle(right)) {
        right->next = *stack;
        *stack = right;
    }
    if (isneedhandle(left)) {
        left->next = *stack;
        *stack = left;
    }
}

static void genlabel(ModuleFunc *func, Node *node, Node **stack)
{
    InstGenData *data = markrule(func, node, true);

    EMIT(&func->arena, data,
         "lea (%s), %%r",
         vreg_unused(), vreg_unused(), data->vreg,
         node->attr.label.name);
}

static void genimm(ModuleFunc *func, Node *node, Node **stack)
{
    InstGenData *data = markrule(func, node, true);

    EMIT(&func->arena, data,
         "mov $%ld, %%r",
         vreg_unused(), vreg_unused(), data->vreg,
         node->attr.imm);
}

static void completion(Inst *inst, InstGenData *data)
{
    /* 通过记录的指令数据进行补全（之前没有填写的寄存器） */
    for (int i = 0; i < 3; i++) {
        if (vreg_isplaceholder(inst->u.vreg[i])) {
            assert(data->nodelist.size != 0);

            InstGenData *childdata = nodelist_pop(&data->nodelist)->data;
            inst->u.vreg[i] = childdata->vreg;
        }
    }

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
        completion(inst, data);

        /* 2. 插入block */
        list_append(&block->insts, &inst->link);

        /* debug */
        /* inst_dprint(stdout, inst); */
    }
}
