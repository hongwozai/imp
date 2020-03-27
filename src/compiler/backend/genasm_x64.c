#include <stdarg.h>
#include <stdlib.h>
#include "utils/bit.h"
#include "genasm.h"
#include "target_x64.h"

static Target *target = &target_x64;

static void emit(FILE *out, const char *fmt, ...)
{
    va_list ap;
    if (!strchr(fmt, ':')) {
        fprintf(out, "    ");
    }
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);
    fprintf(out, "\n");
}

static void emitinst(FILE *out, Inst *inst)
{
    fprintf(out, "    ");
    inst_dprint(out, inst);
}

static void linear(FILE *out, ModuleFunc *func)
{
    uint8_t *bitset = malloc((ptrvec_count(&func->blocks) << 3) + 1);

    List stack;
    list_init(&stack);
    list_append(&stack, &func->start->link);

    /* 广度优先 */
    while (!list_empty(&stack)) {
        Block *block = container_of(list_pop(&stack), Block, link);
        bit_set(bitset, block->id);

        /* 输出 */
        emit(out, "%s:", block->name);
        list_foreach(pos, block->insts.first) {
            Inst *inst = container_of(pos, Inst, link);
            emitinst(out, inst);
        }

        /* push */
        ptrvec_foreach(iter, &block->succs) {
            Block *child = ptrvec_get(&block->succs, iter);

            /* 如果当前已经遍历过，或者已经在列表中了，那么不加入 */
            if (bit_check(bitset, child->id)) {
                continue;
            }

            list_append(&stack, &child->link);
        }
    }
    free(bitset);
}

static bool savecallee(FILE *out, ModuleFunc *func)
{
    int num = 0;
    for (int i = 0; i < target->regnum; i++) {
        if (bit_check(func->calleeset, i) &&
            target->regs[i].type == kCalleeSave) {
            emit(out, "pushq %s", target->regs[i].rep);
            num++;
        }
    }
    if ((num & 1) == 1) {
        emit(out, "pushq $0");
        return true;
    }
    return false;
}

static void loadcallee(FILE *out, ModuleFunc *func, bool isalign)
{
    if (isalign) {
        emit(out, "addq $8, %rsp");
    }
    for (int i = target->regnum; i > 0; i--) {
        if (bit_check(func->calleeset, i) &&
            target->regs[i].type == kCalleeSave) {
            emit(out, "popq %s", target->regs[i].rep);
        }
    }
}

static void walk_func(FILE *out, ModuleFunc *func)
{
    bool isalign = false;

    /* 函数准备操作 */
    if (!func->name) {
        emit(out, ".global _main");
        emit(out, "_main:");
    } else {
        emit(out, "%s:", func->name);
    }
    emit(out, "push %rbp");
    emit(out, "movq %rsp, %rbp");

    /* 被调用者保存寄存器 */
    isalign = savecallee(out, func);

    /* 扩展栈大小 */
    emit(out, "subq $0x%02X, %rsp", func->stacksize);

    /* 遍历block，打印指令 */
    linear(out, func);

    /* 恢复被调用者保存的寄存器 */
    loadcallee(out, func, isalign);

    /* 函数结束操作 */
    emit(out, "leaveq");
    emit(out, "ret");
    emit(out, "");
}

void genasm_run(FILE *out, Module *module)
{
    /* 先输出label */
    if (module->labels.size != 0) {
        emit(out, ".section .data");
    }
    list_foreach(pos, module->labels.first) {
        ModuleLabel *label = container_of(pos, ModuleLabel, link);
        emit(out, "%s:", label->name);
        for (size_t i = 0; i < label->len; i++) {
            emit(out, ".byte 0x%X", label->data[i]);
        }
    }

    /* 再遍历函数 */
    emit(out, "");
    emit(out, ".section .text");
    list_foreach(pos, module->funcs.first) {
        ModuleFunc *func = container_of(pos, ModuleFunc, link);
        walk_func(out, func);
    }
}
