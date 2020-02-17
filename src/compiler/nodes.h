#ifndef SRC_COMPILER_NODES_H
#define SRC_COMPILER_NODES_H

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
    kNodePhi,
    kNodeRegion,
    kNodeIf,
    kNodeLoad,
    kNodeStore,
} Opcode;

typedef struct Use {
    ListLink link;
    size_t index;
    Node *node;
} Use;

typedef union NodeAttr {
    size_t imm;
    Object *obj;
    Node *node;
} NodeAttr;

typedef enum WalkMode {
    kModeTop,
    kModeMiddle,
    kModeBottom,
} WalkMode;

struct Node {
    Opcode op;
    PtrVec inputs;
    PtrVec ctrls;
    List uses;
    List ctrluses;
    NodeAttr attr;
    /* 用于遍历 */
    WalkMode mode;
    Node *next;
};

Node* node_new(Arena *arena, Opcode op);
void  node_use(Arena *arena, Node *self, Node *usenode, size_t index, bool isctrl);
void  node_unuse(Node *self, Node *other, size_t index, bool isctrl);
void  node_addinput(Arena *arena, Node *self, Node *other, bool isctrl);
void  node_replaceinput(Arena *arena, Node *self, bool isctrl,
                        size_t index, Node *other);
void  node_removeinput(Arena *arena, Node *self, size_t index, bool isctrl);
bool  node_isctrl(Node *self);
void  node_remove(Node *self);
void  node_verify(Node *self);

#endif /* SRC_COMPILER_NODES_H */