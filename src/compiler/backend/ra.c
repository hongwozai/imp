#include <stdarg.h>
#include "ra.h"
#include "utils/bit.h"
#include "target_x64.h"

#define ROUNDUP(x, n) (((x) + ((n) - 1)) & (~((n) - 1)))

static Target *target = &target_x64;

/* 插入在inst指令之前 */
static Inst* newinst(Block *block, Inst *inst, bool isbefore,
                     Arena *arena,
                     const char *desc,
                     InstReg *op1, InstReg *op2, InstReg *dst,
                     ...)
{
    va_list ap;
    char buf[512];

    va_start(ap, dst);
    vsnprintf(buf, 512, desc, ap);
    va_end(ap);

    Inst *x = inst_new(arena, buf, op1, op2, dst);
    if (isbefore) {
        list_add(&block->insts, inst->link.prev, &x->link);
    } else {
        list_add(&block->insts, &inst->link, &x->link);
    }
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
        instreg_vset(&interval->reg, i);
        interval->start = 0;
        interval->end = 0;
        interval->throughcall = false;
        list_link_init(&interval->link);
        list_link_init(&interval->activelink);
        interval->result.type = kRAUnalloc;
        ptrvec_add(&ra->arena, &ra->intervalmap, interval);
    }
    assert(ptrvec_count(&ra->intervalmap) == regnum);

    ra->regtotal = target->regnum;
    ra->regset = bit_new(&ra->arena, ra->regtotal);

    /* 这里实现要把栈基址寄存器的空间留出来(因为栈向下生长的) */
    ra->stacksize = target->regsize;
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

        switch (interval->reg.type) {
        case kInstRegUnUsed:
        case kInstRegDummy:
            assert(!"bug!");
            break;
        case kInstRegTarget:
            printf("reg: %s", interval->reg.reg->rep);
            break;
        case kInstRegVirtual:
            printf("reg: v%d", interval->reg.vreg);
            break;
        }
        printf(" start: %ld end: %ld\n", interval->start, interval->end);
    }
}

static void addinterval(RA *ra, TargetReg *reg);
static void handlereg(RA *ra, Inst *inst, InstReg *reg)
{
    /* 虚拟寄存器 */
    if (instreg_isvirtual(reg)) {
        /* 找到对应的interval */
        Interval *interval = ptrvec_get(&ra->intervalmap, reg->vreg);

        assert(interval->reg.vreg == reg->vreg);
        interval->end = ra->number;

        if (interval->start == 0) {
            interval->start = ra->number;
            list_append(&ra->originlist, &interval->link);
        }
        return;
    }
}

static void buildinterval(RA *ra, Block *block) {
    /* 遍历每一条指令，对每一条指令的虚拟寄存器进行分析 */
    list_foreach(pos, block->insts.first) {
        Inst *inst = container_of(pos, Inst, link);
        ra->number ++;

        for (int i = 0; i < kInstMaxOp; i++) {
            if (instreg_isdummy(&inst->reg[i]) ||
                instreg_isunused(&inst->reg[i])) {
                continue;
            }

            handlereg(ra, inst, &inst->reg[i]);
        }
    }

    /* 判断interval中是否有call */

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
    uint8_t *bitset = bit_new(&ra->arena, blocknum);

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
static TargetReg* allocarg(RA *ra, ModuleFunc *func, Interval *interval)
{
    /**
     * TODO: 这里未实现调用者保存逻辑和是否有call调用在中间
     * 所以这里先不使用
     */
    int type[] = { kCalleeSave, /* kCallerSave, */ -1 };

    if (interval->throughcall) {
        type[0] = kCalleeSave;
        type[1] = -1;
    }

    for (int t = 0; type[t] >= 0; t++) {
        for (int i = 0; i < ra->regtotal; i++) {
            /* 先处理这个分类的 */
            if (target->regs[i].type != type[t]) {
                continue;
            }

            /* 已经使用 */
            if (bit_check(ra->regset, i)) {
                continue;
            }

            if (target->regs[i].type == kCalleeSave) {
                if (!func->calleeset) {
                    func->calleeset = bit_new(&func->arena, target->regnum);
                }
                bit_set(func->calleeset, i);
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

static void scan(RA *ra, ModuleFunc *func)
{
    list_foreach(pos, ra->originlist.first) {
        Interval *interval = container_of(pos, Interval, link);

        /* 排除过期的 */
        expire_active(ra, interval->start);

        TargetReg *reg = allocarg(ra, func, interval);
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

static void spill(RA *ra, ModuleFunc *func, Block *block,
                  Inst *inst, int pos, InstReg *reg)
{
    /* 该虚拟寄存器对应的结果 */
    Interval *interval = ptrvec_get(&ra->intervalmap, reg->vreg);
    int offset = - func->alignsize - func->calleesize - interval->result.stackpos;

    if (interval->result.type == kRAReg) {
        instreg_talloc(reg, interval->result.reg);
    } else {
        assert(interval->result.type != kRAUnalloc);

        if (pos == kInstOperand1 || pos == kInstOperand2) {
            /* spill代码，从内存中读取到临时寄存器 */
            InstReg op1, op2, dst;
            newinst(block, inst, true, &func->arena,
                    interval->result.stackpos == 0
                    ? "mov (%%1), %%r"
                    : "mov %d(%%1), %%r",
                    instreg_talloc(&op1, &target->regs[RBP]),
                    instreg_unused(&op2),
                    instreg_talloc(&dst, &target->regs[RAX]),
                    offset);

            instreg_talloc(reg, &target->regs[RAX]);
        } else {
            /* 从寄存器中存储到内存 */
            InstReg op1, op2, dst;
            newinst(block, inst, false, &func->arena,
                    interval->result.stackpos == 0
                    ? "mov %%1, (%%r)"
                    : "mov %%1, %d(%%r)",
                    instreg_talloc(&op1, &target->regs[RAX]),
                    instreg_unused(&op2),
                    instreg_talloc(&dst, &target->regs[RBP]),
                    offset);

            instreg_talloc(reg, &target->regs[RAX]);
        }
    }
}

static void completion(RA *ra, ModuleFunc *func, Block *block)
{
    list_safe_foreach(pos, iter, block->insts.first) {
        Inst *inst = container_of(pos, Inst, link);

        /* 对所有的虚拟寄存器循环进行处理 */
        for (int i = 0; i < kInstMaxOp; i++) {
            if (!instreg_isvirtual(&inst->reg[i])) {
                continue;
            }
            assert(inst->reg[i].vreg < ptrvec_count(&ra->intervalmap));
            /* 针对每个虚拟寄存器 */
            spill(ra, func, block, inst, i, &inst->reg[i]);
        }
        /* inst_dprint(stdout, inst); */
    }
}

static void calcstacksize(RA *ra, ModuleFunc *func)
{
    int num = 0;

    if (func->calleeset) {
        for (int i = 0; i < target->regnum; i++) {
            if (bit_check(func->calleeset, i) &&
                target->regs[i].type == kCalleeSave) {
                num++;
            }
        }
    }
    /* 栈还需要将被调者保存的寄存器保存的空间 */
    func->calleesize = num * target->regsize;
    /* 这里让栈按要求的尺寸字节对齐 */
    func->stacksize = ra->stacksize;

    /* 设置对齐字节数 */
    int size = (func->calleesize + func->stacksize) % target->stackalign;
    if (size != 0) {
        func->alignsize = target->stackalign - size;
        assert(func->alignsize != target->regnum);
    }
}

static void walk_func(ModuleFunc *func)
{
    RA ra;
    newra(&ra, func->vregindex);

    /* 1. 线性化CFG，buildinterval */
    linear(&ra, func);

    /* 2. 遍历interval，并进行分配 */
    scan(&ra, func);

    /* 3. 计算栈空间 */
    calcstacksize(&ra, func);

    /* 4. 填回insts */
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
