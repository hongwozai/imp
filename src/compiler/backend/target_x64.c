#include <limits.h>

#include "target.h"
#include "target_x64.h"

static TargetReg x64regs[] = {
    [RAX] = {
        .id = RAX,
        .rep = "%rax",
        .type = kFreeReg,
    },
    [RBX] = {
        .id = RBX,
        .rep = "%rbx",
        .type = kCalleeSave,
    },
    [RCX] = {
        .id = RCX,
        .rep = "%rcx",
        .type = kArg,
    },
    [RDX] = {
        .id = RDX,
        .rep = "%rdx",
        .type = kArg,
    },
    [RSI] = {
        .id = RSI,
        .rep = "%rsi",
        .type = kArg,
    },
    [RDI] = {
        .id = RDI,
        .rep = "%rdi",
        .type = kArg,
    },
    [RBP] = {
        .id = RBP,
        .rep = "%rbp",
        .type = kStackBasePointer,
    },
    [RSP] = {
        .id = RSP,
        .rep = "%rsp",
        .type = kStackPointer,
    },
    [R8] = {
        .id = R8,
        .rep = "%r8",
        .type = kArg,
    },
    [R9] = {
        .id = R9,
        .rep = "%r9",
        .type = kArg,
    },
    [R10] = {
        .id = R10,
        .rep = "%r10",
        .type = kCallerSave,
    },
    [R11] = {
        .id = R11,
        .rep = "%r11",
        .type = kCallerSave,
    },
    [R12] = {
        .id = R12,
        .rep = "%r12",
        .type = kCalleeSave,
    },
    [R13] = {
        .id = R13,
        .rep = "%r13",
        .type = kCalleeSave,
    },
    [R14] = {
        .id = R14,
        .rep = "%r14",
        .type = kCalleeSave,
    },
    [R15] = {
        .id = R15,
        .rep = "%r15",
        .type = kCalleeSave,
    },
};

Target target_x64 = {
    .regsize = 8,
    .regnum  = 16,
    .regs = x64regs,
    .imm_min = INT_MIN,
    .imm_max = INT_MAX,
    .stackalign = 16,
};
