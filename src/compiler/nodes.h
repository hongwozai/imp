#ifndef SRC_COMPILER_NODES_H
#define SRC_COMPILER_NODES_H

#include <stdbool.h>
#include "ptrvec.h"
#include "utils/list.h"
#include "runtime/object.h"

typedef enum Opcode {
    kNodeCallObj,
    kNodeConstObj,
    kNodePhi,
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
    struct ObjAttr {
        Object *obj;
    } objattr;
    struct ImmAttr {
        size_t imm;
    } immattr;
} NodeAttr;

typedef struct Node {
    Opcode op;
    PtrVec inputs;
    PtrVec ctrls;
    List uses;
    List ctrlout;
    NodeAttr attr;
} Node;

Node* node_new(Arena *arena, Opcode op);
void  node_addinput(Arena *arena, Node *self, Node *other, bool isctrl);
void  node_removeinput(Arena *arena, Node *self, size_t index, bool isctrl);
bool  node_isctrl(Node *self);
void  node_replaceinput(Arena *arena, Node *self, size_t index, Node *other);
void  node_use(Arena *arena, Node *self, Node *other, bool isctrl);
void  node_unuse(Arena *arena, Node *self, Node *other, bool isctrl);
void  node_remove(Node *self);
void  node_verify(Node *self);

#endif /* SRC_COMPILER_NODES_H */