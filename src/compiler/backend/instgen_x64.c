#include <assert.h>
#include <stdarg.h>
#include "utils/bit.h"
#include "insts.h"
#include "instgen.h"
#include "instgen_internal.h"
#include "nodes.h"
#include "target_x64.h"

#define EMIT(arena, data, fmt, args...)                 \
    list_append(&data->insts,                           \
                &newinst3(arena, fmt, ##args)->link)

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
static void genreturn(ModuleFunc *func, Node *node, Node **stack);
static void emit(ModuleFunc *func, Block *block, Node *node);

static InstGenOp ops[] = {
    [kNodeLoad]   = { .gen = genload,   .emit = emit },
    [kNodeAdd]    = { .gen = genadd,    .emit = emit },
    [kNodeCall]   = { .gen = gencall,   .emit = emit },
    [kNodeLabel]  = { .gen = genlabel,  .emit = emit },
    [kNodeImm]    = { .gen = genimm,    .emit = emit },
    [kNodeReturn] = { .gen = genreturn, .emit = emit }
};

InstGenOp *instgenop_x64 = ops;

static Target *target = &target_x64;

static Inst *newinst3(Arena *arena, const char *desc,
                      InstReg *op1, InstReg *op2, InstReg *dst,
                      ...)
{
    va_list ap;
    char buf[512];

    va_start(ap, dst);
    vsnprintf(buf, 512, desc, ap);
    va_end(ap);

    return inst_new(arena, buf, op1, op2, dst);
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
    data->vreg = isalloc ? vreg_alloc(func) : -1;

    node->data = data;
    return data;
}

static void markused(ModuleFunc *func, Node *node)
{
    InstGenData *data = arena_malloc(&func->arena, sizeof(InstGenData));
    data->type = kInstDataUsed;
    data->vreg = -1;
    list_init(&data->insts);
    list_init(&data->nodelist);

    node->data = data;
}

/**
 * 使用场景：
 * 是否可以往队列中插入
 */
static bool isneedhandle(Node *node)
{
    return !instgen_isqueue(node) && !node->data;
}

/**
 * 该函数使用场景：
 * 在判断子节点是否可以与当前节点为一个覆盖(无人处理的节点)
 */
static bool cancover(Node *node)
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
    InstReg op1, op2, dst;
    assert(ptrvec_count(&node->inputs) == 1);

    InstGenData *data = markrule(func, node, true);
    Node *child = ptrvec_get(&node->inputs, 0);

    /* bingo! (Load (Add reg imm)) */
    if (child->op == kNodeAdd && cancover(child)) {
        /* (Add reg imm) or (Add imm reg) */
        assert(ptrvec_count(&child->inputs) == 2);

        Node *left = ptrvec_get(&child->inputs, 0);
        Node *right = ptrvec_get(&child->inputs, 1);

        /**
         * 继续进行树覆盖时，必须保证接下来的节点可处理
         * 不可处理的节点一定是在队列中已经化为了其他节点的处理对象
         */
        if ((left->op == kNodeImm && cancover(left)) ||
            (right->op == kNodeImm && cancover(left))) {

            Node *imm = (left->op == kNodeImm) ? left : right;
            Node *reg = (left->op == kNodeImm) ? right : left;

            EMIT(&func->arena, data,
                 "mov %ld(%%1), %%r",
                 instreg_dummy(&op1),
                 instreg_unused(&op2),
                 instreg_vset(&dst, data->vreg),
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
         instreg_dummy(&op1),
         instreg_unused(&op2),
         instreg_vset(&dst, data->vreg));

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
    InstReg op1, op2, dst;
    TargetReg *argreg = NULL;

    switch (iter) {
    case 1: argreg = &target->regs[RDI]; break;
    case 2: argreg = &target->regs[RSI]; break;
    case 3: argreg = &target->regs[RDX]; break;
    case 4: argreg = &target->regs[RCX]; break;
    case 5: argreg = &target->regs[R8]; break;
    case 6: argreg = &target->regs[R9]; break;
    }

    if (iter <= 6) {
        assert(argreg);

        switch (node->op) {
        case kNodeImm: {
            EMIT(&func->arena, data, "mov $%ld, %%r",
                 instreg_unused(&op1),
                 instreg_unused(&op2),
                 instreg_talloc(&dst, argreg),
                 node->attr.imm);
            markused(func, node);
            return false;
        }
        case kNodeLabel: {
            EMIT(&func->arena, data, "lea (%s), %%r",
                 instreg_unused(&op1),
                 instreg_unused(&op2),
                 instreg_talloc(&dst, argreg),
                 node->attr.label.name);
            markused(func, node);
            return false;
        }
        default: {
            EMIT(&func->arena, data, "mov %%1, %%r",
                 instreg_dummy(&op1),
                 instreg_unused(&op2),
                 instreg_talloc(&dst, argreg));

            nodelist_append(&func->arena, &data->nodelist,  node);
            return true;
        }
        }
    }

    if (node->op == kNodeImm) {
        EMIT(&func->arena, data, "push %ld",
             instreg_unused(&op1), instreg_unused(&op2), instreg_unused(&dst),
             node->attr.imm);
        return false;
    }

    EMIT(&func->arena, data, "push %%1",
         instreg_dummy(&op1), instreg_unused(&op2), instreg_unused(&dst));

    nodelist_append(&func->arena, &data->nodelist, node);
    return true;
}

static void gencall(ModuleFunc *func, Node *node, Node **stack)
{
    InstReg op1, op2, dst;
    Node *funcnode = NULL;
    InstGenData *data = markrule(func, node, node->uses.size != 0);

    ptrvec_foreach(iter, &node->inputs) {
        /* 反向匹配参数 */
        size_t argindex = ptrvec_count(&node->inputs) - iter - 1;
        Node *parent = ptrvec_get(&node->inputs, argindex);

        /* 函数地址 */
        if (argindex == 0) {
            funcnode = parent;

            if (parent->op == kNodeLabel && cancover(parent)) {
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
        EMIT(&func->arena, data, "call (%s)",
             instreg_unused(&op1),
             instreg_unused(&op2),
             instreg_unused(&dst),
             funcnode->attr.label.name);
        markused(func, funcnode);
    } else {
        /* (Call vreg ...) */
        EMIT(&func->arena, data, "call *%%1",
             instreg_dummy(&op1),
             instreg_unused(&op2),
             instreg_unused(&dst));

        nodelist_append(&func->arena, &data->nodelist,  funcnode);
    }

    /* 构造冲突集合，call指令与callersave寄存器冲突 */
    Inst *inst = container_of(data->insts.last, Inst, link);
    inst->iscall = true;

    /* 当该调用无人使用时，不申请结果寄存器 */
    if (node->uses.size != 0) {
        EMIT(&func->arena, data,
             "mov %%1, %%r",
             instreg_talloc(&op1, &target->regs[RAX]),
             instreg_unused(&op2),
             instreg_vset(&dst, data->vreg));
    }
}

static void genadd(ModuleFunc *func, Node *node, Node **stack)
{
    InstReg op1, op2, dst;

    /* child */
    assert(ptrvec_count(&node->inputs) == 2);
    Node *left = ptrvec_get(&node->inputs, 0);
    Node *right = ptrvec_get(&node->inputs, 1);

    /* insert instrutions */
    InstGenData *data = markrule(func, node, true);

    /* emit */
    EMIT(&func->arena, data, "lea (%%1, %%2, 1), %%r",
         instreg_dummy(&op1),
         instreg_dummy(&op2),
         instreg_vset(&dst, data->vreg));
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
    InstReg op1, op2, dst;
    InstGenData *data = markrule(func, node, true);

    EMIT(&func->arena, data,
         "lea (%s), %%r",
         instreg_unused(&op1),
         instreg_unused(&op2),
         instreg_vset(&dst, data->vreg),
         node->attr.label.name);
}

static void genimm(ModuleFunc *func, Node *node, Node **stack)
{
    InstReg op1, op2, dst;
    InstGenData *data = markrule(func, node, true);

    EMIT(&func->arena, data,
         "mov $%ld, %%r",
         instreg_unused(&op1),
         instreg_unused(&op2),
         instreg_vset(&dst, data->vreg),
         node->attr.imm);
}

static void genreturn(ModuleFunc *func, Node *node, Node **stack)
{
    InstReg op1, op2, dst;
    InstGenData *data = markrule(func, node, false);

    assert(node->uses.size == 0);
    assert(ptrvec_count(&node->inputs) == 1);

    Node *retval = ptrvec_get(&node->inputs, 0);

    /* 处理返回值 */
    if (isneedhandle(retval)) {
        retval->next = *stack;
        *stack = retval;
    }
    nodelist_append(&func->arena, &data->nodelist,  retval);

    EMIT(&func->arena, data,
         "mov %%1, %%r",
         instreg_dummy(&op1),
         instreg_unused(&op2),
         instreg_talloc(&dst, &target->regs[RAX]));
}

static void completion(Inst *inst, InstGenData *data)
{
    /* 通过记录的指令数据进行补全（之前没有填写的寄存器） */
    for (int i = 0; i < kInstMaxOp; i++) {
        if (instreg_isdummy(&inst->reg[i])) {
            Node *node;
            InstGenData *childdata;

            assert(data->nodelist.size != 0);

            node = nodelist_pop(&data->nodelist);
            childdata = node->data;
            instreg_vset(&inst->reg[i], childdata->vreg);
        }
    }

}

static void emit(ModuleFunc *func, Block *block, Node *node)
{
    InstGenData *data = node->data;
    assert(data->type == kInstDataRule);

    /* for debug */
    /* printf("debug: "); */
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
