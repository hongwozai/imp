#ifndef IMP_SRC_COMPILER_BACKEND_INSTS_H
#define IMP_SRC_COMPILER_BACKEND_INSTS_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "utils/list.h"
#include "utils/arena.h"
#include "ptrvec.h"
#include "target.h"
#include "analysis.h"

typedef int VirtualReg;

enum InstRegPos {
    kInstOperand1 = 0,
    kInstOperand2,
    kInstDst,
    /* 代表这最大数量 */
    kInstMaxOp,
};

/* 此处封装虚拟与非虚拟 */
typedef struct InstReg {
    enum  {
        /* 虚拟的 */
        kInstRegVirtual,
        /* 目标机器的 */
        kInstRegTarget,
        /* 该位置没有使用 */
        kInstRegUnUsed,
        /* 该位置占位符，等待之后填入 */
        kInstRegDummy,
    } type;
    TargetReg  *reg;
    VirtualReg  vreg;
} InstReg;

typedef struct Inst {
    ListLink link;
    /* 指令内容 */
    char *desc;
    /* 对指令内容的标注 */
    bool iscall;
    /* 指令使用的寄存器 */
    InstReg reg[kInstMaxOp];
} Inst;

typedef struct Block {
    /* 名称，生成时的标签 */
    char *name;
    /* 指令 */
    List insts;
    /* 前驱 */
    PtrVec preds;
    /* 后继 */
    PtrVec succs;
    /* 该block对应的region */
    Node *region;
    /* 遍历使用: 索引号 */
    size_t id;
    /* 遍历使用： 队列/栈 */
    ListLink link;
} Block;

typedef struct ModuleFunc {
    ListLink link;
    Arena arena;
    PtrVec blocks;
    Block *start;
    /* 总共使用了多少寄存器（可能有空洞） */
    size_t vregindex;
    /* 栈深度，寄存器分配之后填入 */
    int stacksize;
    /* 该函数使用了哪些调用者保存的寄存器 */
    uint8_t *calleeset;
} ModuleFunc;

typedef struct ModuleLabel {
    ListLink link;
    char *name;
    char *data;
    size_t len;
} ModuleLabel;

typedef struct Module {
    Arena arena;
    List labels;
    List funcs;
} Module;

Inst*    inst_new(Arena *arena, const char *desc,
                  InstReg *op1, InstReg *op2, InstReg *dst);
void     inst_dprint(FILE *out, Inst *inst);

VirtualReg vreg_alloc(ModuleFunc *func);

InstReg *instreg_vset(InstReg *reg, VirtualReg vreg);
InstReg *instreg_talloc(InstReg *reg, TargetReg *treg);
InstReg *instreg_unused(InstReg *reg);
InstReg *instreg_dummy(InstReg *reg);
bool     instreg_isdummy(InstReg *reg);
bool     instreg_isvirtual(InstReg *reg);
bool     instreg_isunused(InstReg *reg);
bool     instreg_istarget(InstReg *reg);

#endif /* IMP_SRC_COMPILER_BACKEND_INSTS_H */
