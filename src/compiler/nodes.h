#ifndef IMP_SRC_COMPILER_NODES_H
#define IMP_SRC_COMPILER_NODES_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "ptrvec.h"
#include "utils/list.h"
#include "utils/arena.h"
#include "runtime/object.h"

typedef struct Node Node;

typedef enum Opcode {
    kNodeCallObj,
    kNodeConstObj,
    kNodeGlobalObj,
    kNodeImm,
    kNodeCall,
    kNodeReturn,
    /* load读取一个寄存器长度的数据 */
    kNodeLoad,
    /* store存放一个寄存器长度的数据 */
    kNodeStore,
    kNodeAdd,
    kNodeShl,
    kNodeLabel,
    kNodePhi,
    kNodeRegion,
    kNodeIf,
} Opcode;

typedef struct Use {
    ListLink link;
    size_t index;
    Node *node;
} Use;

typedef union NodeAttr {
    intptr_t imm;
    Object *obj;
    Node *node;

    struct LabelData {
        char *name;
        char *data;
        size_t datalen;
    } label;

} NodeAttr;

/**
 * 入队了为Middle状态
 * 子访问完为Visit状态(待参观的状态，等待子节点处理完了)
 * 访问完自己是Bottom状态
 */
typedef enum WalkMode {
    kModeTop,
    kModeMiddle,
    kModeVisit,
    kModeBottom,
} WalkMode;

struct Node {
    Opcode op;
    PtrVec inputs;
    PtrVec ctrls;
    List uses;
    List ctrluses;
    NodeAttr attr;
    /* 用于遍历，记录是否遍历完成 */
    WalkMode mode;
    /* 用于记录遍历的中间节点 */
    Node *next;
    /* 用于存储数据，避免映射表 */
    void *data;
};

Node* node_new(Arena *arena, Opcode op);
Node* node_newlabel(Arena *arena, char *name, char *data, size_t len);
void  node_use(Arena *arena, Node *self, Node *usenode, size_t index, bool isctrl);
void  node_unuse(Node *self, Node *other, size_t index, bool isctrl);
void  node_addinput(Arena *arena, Node *self, Node *other, bool isctrl);
void  node_replaceinput(Arena *arena, Node *self, bool isctrl,
                        size_t index, Node *other);
void  node_vreplace(Arena *arena, Node *self, Node *other);
void  node_dprint(FILE *out, Node *self);

static inline
void  node_setmode(Node* self, WalkMode mode) { self->mode = mode; }
static inline
void  node_setop(Node* self, Opcode op) { self->op = op; }

#endif /* IMP_SRC_COMPILER_NODES_H */
