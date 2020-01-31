#ifndef SRC_COMPILER_IR_H
#define SRC_COMPILER_IR_H

typedef struct Operator {
    const char *name;
} Operator;

typedef struct IRNode {
    Operator op;
    Node *inputs;
} IRNode;

#endif /* SRC_COMPILER_IR_H */