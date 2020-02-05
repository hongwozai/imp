#ifndef SRC_COMPILER_X64_X64_H
#define SRC_COMPILER_X64_X64_H

#include <stddef.h>

typedef struct TargetReg {
    const char *name;
    enum {
        kCalleeSave,
        kCallerSave,
        kArg,
    } type;
} TargetReg;

typedef struct Target {
    /* 寄存器大小 */
    size_t regsize;
    /* 寄存器个数 */
    size_t regnum;
    /* 寄存器 */
    TargetReg *regs;

    /* 立即数的范围 */
    ssize_t imm_min;
    ssize_t imm_max;

    /* emit */
    void (*emit)(int op, ...);
} Target;

#endif /* SRC_COMPILER_X64_X64_H */