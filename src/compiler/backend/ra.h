#ifndef IMP_SRC_COMPILER_BACKEND_RA_H
#define IMP_SRC_COMPILER_BACKEND_RA_H

#include "utils/arena.h"
#include "utils/bit.h"
#include "compiler/ptrvec.h"
#include "insts.h"

typedef struct RAResult {
    /* 这里记录分配的结果 */
    enum {
        kRAUnalloc,
        kRAReg,
        kRAStack,
    } type;
    TargetReg *reg;
    int stackpos;
} RAResult;

typedef struct Interval {
    VirtualReg vreg;
    size_t start;
    size_t end;
    ListLink link;
    ListLink activelink;

    /* 记录分配的结果 */
    RAResult result;
} Interval;

/* 每一个函数有一个结构体 */
typedef struct RA {
    Arena arena;
    size_t number;
    PtrVec intervalmap;
    List originlist;
    List activelist;

    /* 当前已经使用的寄存器总个数 */
    size_t regused;
    /* 当前哪些寄存器使用了 */
    uint8_t *regset;
    /* 当前总共可以使用的寄存器个数 */
    size_t regtotal;
    /* 当前栈已经用了多长 */
    int stacksize;
} RA;

/* register allocation */
void ra_run(Module *module);

#endif /* IMP_SRC_COMPILER_BACKEND_RA_H */
