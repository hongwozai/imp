#ifndef SRC_COMPILER_INSTS_H
#define SRC_COMPILER_INSTS_H

#include "utils/list.h"

typedef enum InstCode {
    kInstLoad,
    kInstStore,
    kInstJump,
} InstCode;

typedef struct Inst {
    InstCode code;
    size_t operand1;
    size_t operand2;
    size_t dst;
    ListLink link;
} Inst;

typedef struct Block {
    PtrVec predecessors;
    PtrVec successors;
    List insts;
} Block;

#endif /* SRC_COMPILER_INSTS_H */