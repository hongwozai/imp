#ifndef IMP_SRC_COMPILER_BACKEND_INSTGEN_INTERNAL_H
#define IMP_SRC_COMPILER_BACKEND_INSTGEN_INTERNAL_H

/**
 * 该文件对内，给各个架构进行开放
 */

#include "utils/arena.h"
#include "utils/list.h"
#include "nodes.h"
#include "insts.h"

typedef struct NodeList {
    Node *node;
    ListLink link;
} NodeList;

typedef struct InstGenData {
    enum {
        /* 当前节点已经被分配为规则的根节点了 */
        kInstDataRule,
        /* 当前节点已经被使用为规则的子节点
           （非根节点，并且不分配寄存器等） */
        kInstDataUsed,
    } type;
    /* dst寄存器的值 */
    VirtualReg vreg;
    /* 指针列表 */
    List insts;
    /* 记录当前需要的指针（尚未分配规则的指针） */
    List nodelist;
} InstGenData;

typedef struct InstGenOp {
    void (*gen)(ModuleFunc *func, Node *node, Node **stack);
    void (*emit)(ModuleFunc *func, Block *block, Node *node);
} InstGenOp;

/* Inst* (*instgen_spill)(ModuleFunc *func, Block *block, */
/*                TargetReg *op1, TargetReg *op2, TargetReg *dst); */

bool instgen_isqueue(Node *node);

#endif /* IMP_SRC_COMPILER_BACKEND_INSTGEN_INTERNAL_H */
