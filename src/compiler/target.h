#ifndef IMP_SRC_COMPILER_TARGET_H
#define IMP_SRC_COMPILER_TARGET_H

#include <stddef.h>
#include <stdint.h>
#include "utils/arena.h"

typedef struct TargetReg {
    int id;
    const char *rep;
    enum {
        kCalleeSave,
        kCallerSave,
        /* 自由寄存器 */
        kFreeReg,
        /* 用作参数的寄存器 */
        kArg,
        /* 栈指针 */
        kStackPointer,
        /* 栈基址指针 */
        kStackBasePointer,
    } type;
} TargetReg;

typedef struct Target {
    /* 寄存器大小，单位字节 */
    size_t regsize;
    /* 寄存器个数 */
    size_t regnum;
    /* 寄存器 */
    TargetReg *regs;

    /* 立即数的范围 */
    intptr_t imm_min;
    intptr_t imm_max;

    /* 栈字节对齐数 */
    size_t stackalign;
} Target;

#endif /* IMP_SRC_COMPILER_TARGET_H */
