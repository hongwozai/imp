#include <stdarg.h>
#include "ra.h"
#include "utils/bit.h"
#include "target_x64.h"

Target *target = &target_x64;

/* 插入在inst指令之前 */
static Inst* newinst(Block *block, Inst *inst, Arena *arena,
                     const char *desc,
                     TargetReg *op1, TargetReg *op2, TargetReg *dst,
                     ...)
{
    va_list ap;
    char buf[512];

    va_start(ap, dst);
    vsnprintf(buf, 512, desc, ap);
    va_end(ap);

    Inst *x = inst_new(arena, buf, op1, op2, dst);
    list_add(&block->insts, inst->link.prev, &x->link);
    return x;
}

static void newra(RA *ra, size_t regnum)
{
    ra->number = 0;
    arena_init(&ra->arena, 1024);
    list_init(&ra->originlist);
    list_init(&ra->activelist);
    ptrvec_init(&ra->arena, &ra->intervalmap, regnum);

    /* 构造interval结构体 */
    for (size_t i = 0; i < regnum; i++) {
        /* vreg -> interval */
        Interval *interval = arena_malloc(&ra->arena, sizeof(Interval));
        interval->vreg = i;
        interval->start = 0;
        interval->end = 0;
        list_link_init(&interval->link);
        list_link_init(&interval->activelink);
        interval->result.type = kRAUnalloc;
        ptrvec_add(&ra->arena, &ra->intervalmap, interval);
    }
    assert(ptrvec_count(&ra->intervalmap) == regnum);

    ra->regused = 0;
    ra->regtotal = target->regnum;
    ra->regset = arena_malloc(&ra->arena, (ra->regtotal << 3) + 1);
    ra->stacksize = 0;
}

static void freera(RA *ra)
{
    arena_dispose(&ra->arena);
}

static void intervalprint(List *list)
{
    /* debug */
    list_foreach(pos, list->first) {
        Interval *interval = container_of(pos, Interval, link);
        printf("reg: v%d, start: %ld end: %ld\n",
               interval->vreg, interval->start, interval->end);
    }
}

static void buildinterval(RA *ra, Block *block)
{
    /* 遍历每一条指令，对每一条指令的虚拟寄存器进行分析 */
    list_foreach(pos, block->insts.first) {
        Inst *inst = container_of(pos, Inst, link);
        ra->number ++;

        if (!inst->isvirtual) {
            continue;
        }
        for (int i = 0; i < 3; i++) {
            assert(inst->u.vreg[i] != vreg_placeholder());
            if (inst->u.vreg[i] == vreg_unused()) {
                continue;
            }

            /* 找到对应的interval */
            assert(ptrvec_count(&ra->intervalmap) != inst->u.vreg[i]);
            Interval *interval = ptrvec_get(&ra->intervalmap,
                                            inst->u.vreg[i]);

            assert(interval->vreg == inst->u.vreg[i]);
            interval->end = ra->number;

            if (interval->start == 0) {
                interval->start = ra->number;
                list_append(&ra->originlist, &interval->link);
            }
        }
    }

    /* debug */
    /* intervalprint(&ra->originlist); */
}

/**
 * 这里使用深度遍历，尽管这个不是最好的
 */
static void linear(RA *ra, ModuleFunc *func)
{
    /* 位图 */
    size_t blocknum = ptrvec_count(&func->blocks);
    uint8_t *bitset = arena_malloc(&ra->arena, (blocknum << 3) + 1);

    List stack;
    list_init(&stack);

    assert(func->start);
    list_push(&stack, &func->start->link);
    while (!list_empty(&stack)) {
        Block *block = container_of(list_pop(&stack), Block, link);
        bit_set(bitset, block->id);

        /* build interval */
        buildinterval(ra, block);

        /* push */
        ptrvec_foreach(iter, &block->succs) {
            Block *child = ptrvec_get(&block->succs,
                                      ptrvec_count(&block->succs) - iter - 1);

            /* 如果当前已经遍历过，或者已经在列表中了，那么不加入 */
            if (bit_check(bitset, child->id)) {
                continue;
            }

            list_push(&stack, &child->link);
        }
    }
}

static int allocstack(RA *ra)
{
    int stackpos = ra->stacksize;
    ra->stacksize += target->regsize;
    return stackpos;
}

/* 分配寄存器 */
static TargetReg* allocarg(RA *ra)
{
    int type[2] = { kCalleeSave, kCallerSave };

    for (int t = 0; t < 2; t++) {
        for (int i = 0; i < ra->regtotal; i++) {
            /* 这里只使用kCalleeSave kCallerSave的寄存器 */
            if (target->regs[i].type != type[t]) {
                continue;
            }
            /* 已经使用 */
            if (bit_check(ra->regset, i)) {
                continue;
            }
            bit_set(ra->regset, i);
            return &target->regs[i];
        }
    }
    return NULL;
}

static void freearg(RA *ra, TargetReg *reg)
{
    assert(reg);
    bit_clear(ra->regset, reg->id);
    ra->regused --;
}

static void expire_active(RA *ra, size_t now)
{
    list_safe_foreach(pos, iter, ra->activelist.first) {
        Interval *interval = container_of(pos, Interval, activelink);
        if (interval->end < now) {
            if (interval->result.type == kRAReg) {
                /* expire */
                list_del(&ra->activelist, &interval->activelink);
                /* free reg */
                freearg(ra, interval->result.reg);
            }
        }
    }
}

static void scan(RA *ra)
{
    list_foreach(pos, ra->originlist.first) {
        Interval *interval = container_of(pos, Interval, link);
        /* 排除过期的 */
        expire_active(ra, interval->start);

        TargetReg *reg = allocarg(ra);
        if (reg) {
            /* 还有空余寄存器，那么分配寄存器 */
            interval->result.type = kRAReg;
            interval->result.reg = reg;
            list_append(&ra->activelist, &interval->activelink);
        } else {
            /* 如果寄存器用完，找一个结束时间最后的 */
            interval->result.type = kRAStack;
            interval->result.stackpos = allocstack(ra);
        }
    }
}

static void completion(RA *ra, ModuleFunc *func, Block *block)
{
    list_safe_foreach(pos, iter, block->insts.first) {
        Inst *inst = container_of(pos, Inst, link);
        if (!inst->isvirtual) { continue; }

        TargetReg *regs[3] = { NULL, NULL, NULL };

        /* 对所有的虚拟寄存器循环进行处理 */
        for (int i = 0; i < 3; i++) {
            if (inst->u.vreg[i] == vreg_unused()) {
                continue;
            }
            assert(inst->u.vreg[i] != vreg_placeholder());
            assert(inst->u.vreg[i] < ptrvec_count(&ra->intervalmap));

            /* 该虚拟寄存器对应的结果 */
            Interval *interval = ptrvec_get(&ra->intervalmap,
                                            (size_t)inst->u.vreg[i]);

            if (interval->result.type == kRAReg) {
                regs[i] = interval->result.reg;
            } else {
                assert(interval->result.type != kRAUnalloc);
                /* spill代码，从内存中读取到临时寄存器 */
                newinst(block, inst, &func->arena,
                        "mov %d(%%1), %%r",
                        &target->regs[StackBasePointer],
                        NULL,
                        &target->regs[FreeReg],
                        -interval->result.stackpos);
                regs[i] = &target->regs[FreeReg];
            }
        }

        inst->isvirtual = false;
        inst->u.reg[0] = regs[0];
        inst->u.reg[1] = regs[1];
        inst->u.reg[2] = regs[2];

        /* inst_dprint(stdout, inst); */
    }
}

static void walk_func(ModuleFunc *func)
{
    RA ra;
    newra(&ra, func->vregindex);

    /* 1. 线性化CFG，buildinterval */
    linear(&ra, func);

    /* 2. 遍历interval，并进行分配 */
    scan(&ra);

    /* 3. 填回insts */
    ptrvec_foreach(iter, &func->blocks) {
        Block *block = ptrvec_get(&func->blocks, iter);
        completion(&ra, func, block);
    }

    freera(&ra);
}

void ra_run(Module *module)
{
    list_foreach(pos, module->funcs.first) {
        ModuleFunc *func = container_of(pos, ModuleFunc, link);
        walk_func(func);
    }
}
