#ifndef IMP_SRC_COMPILER_BACKEND_TARGET_X64_H
#define IMP_SRC_COMPILER_BACKEND_TARGET_X64_H

#include "target.h"

enum {
    RAX = 0,
    RBX,
    RCX,
    RDX,
    RSI,
    RDI,
    RBP,
    RSP,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
    FreeReg = RAX,
    StackBasePointer = RBP,
    StackPointer = RSP,
};

extern Target target_x64;

#endif /* IMP_SRC_COMPILER_BACKEND_TARGET_X64_H */