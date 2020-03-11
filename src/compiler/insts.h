#ifndef IMP_SRC_COMPILER_INSTS_H
#define IMP_SRC_COMPILER_INSTS_H

#include "utils/list.h"
#include "ptrvec.h"

typedef enum InstCode {
    kInstLoad,
    kInstStore,
    kInstJump,
    kInstCall,
    kInstSetArg,
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

void geninst_init();
void geninst_run();

#endif /* IMP_SRC_COMPILER_INSTS_H */