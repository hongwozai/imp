#ifndef IMP_SRC_COMPILER_BACKEND_INSTS_H
#define IMP_SRC_COMPILER_BACKEND_INSTS_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "utils/list.h"
#include "utils/arena.h"
#include "compiler/ptrvec.h"
#include "compiler/target.h"
#include "compiler/analysis.h"

typedef int VirtualReg;

enum InstRegPos {
    kInstOperand1 = 0,
    kInstOperand2,
    kInstDst,
};

typedef struct Inst {
    ListLink link;
    char *desc;
    bool isvirtual;
    union {
        /* operand1 [operand2] dst */
        TargetReg *reg[3];
        /* virtual reg: operand1 [operand2] dst */
        VirtualReg vreg[3];
    } u;
} Inst;

typedef struct Block {
    char *name;
    PtrVec preds;
    PtrVec succs;
    List insts;
    /* 该block对应的region */
    Node *region;
} Block;

typedef struct ModuleFunc {
    ListLink link;
    Arena arena;
    PtrVec blocks;
    Block *start;
    /* 总共使用了多少寄存器 */
    size_t vregindex;
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

Inst* inst_newv(Arena *arena, const char *desc,
                VirtualReg operand1, VirtualReg operand2, VirtualReg dst);

void inst_dprint(FILE *out, Inst *inst);

/* inline */
static inline VirtualReg vreg_alloc(ModuleFunc *func) {
    return func->vregindex++;
}
static inline VirtualReg vreg_unused() { return -1; }
static inline VirtualReg vreg_placeholder() { return -2; }
static inline bool vreg_isused(VirtualReg reg) {
    return reg >= 0 || reg == vreg_placeholder();
}
static inline bool vreg_isplaceholder(VirtualReg r) { return r == -2; }

#endif /* IMP_SRC_COMPILER_BACKEND_INSTS_H */
