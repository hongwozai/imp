#ifndef IMP_SRC_COMPILER_BACKEND_RA_H
#define IMP_SRC_COMPILER_BACKEND_RA_H

#include "utils/arena.h"
#include "utils/bit.h"
#include "ptrvec.h"
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
    /* 代表的指令(virtual, target两种) */
    InstReg reg;
    /* 是否跨过了调用指令，跨过调用指令不能够分配callersave寄存器 */
    bool throughcall;
    /* 区间的开始与结束 */
    size_t start;
    size_t end;
    /* originlist的连接器 */
    ListLink link;
    /* activelist的连接器 */
    ListLink activelink;
    /* 记录分配的结果 */
    RAResult result;
} Interval;

/* 每一个函数有一个结构体 */
typedef struct RA {
    Arena arena;
    /* 指令计数（代表下一条指令的编号） */
    size_t number;
    /* 用虚拟寄存器的编号映射区间，以及分配的结果 */
    PtrVec intervalmap;
    /* 所有的区间，按开始顺序 */
    List originlist;
    /* 当前活跃的区间 */
    List activelist;
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
