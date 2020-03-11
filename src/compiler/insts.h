#ifndef IMP_SRC_COMPILER_INSTS_H
#define IMP_SRC_COMPILER_INSTS_H

#include "utils/list.h"
#include "ptrvec.h"
#include "analysis.h"

typedef enum InstCode {
    kInstLoadReg,
    kInstLoadImm,
    kInstLoadLabel,
    kInstStore,
    kInstJump,
    kInstCall,
    kInstSetArg,
    kInstAdd,
} InstCode;

typedef struct InstReg {
    enum {
        kInstRegNone,           /* 不使用 */
        kInstRegReg,            /* 寄存器 */
        kInstRegImm,            /* 立即数 */
        kInstRegLabel,          /* 标签index */
    } type;
    size_t value;
} InstReg;

typedef struct Inst {
    ListLink link;
    InstCode code;
    InstReg operand1;
    InstReg operand2;
    InstReg dst;
} Inst;

typedef struct Block {
    PtrVec preds;
    PtrVec succs;
    List insts;
} Block;

void geninst_init();
void geninst_run(Analy *analy);
void geninst_print();
void inst_print();

#endif /* IMP_SRC_COMPILER_INSTS_H */
