#ifndef SRC_COMPILER_OPERATOR_H
#define SRC_COMPILER_OPERATOR_H

typedef struct IrOperator {
    const char *name;
} IrOperator;

/* lisp */
extern IrOperator op_constobj;
extern IrOperator op_globalobj;
extern IrOperator op_callobj;

/* low */
extern IrOperator op_start;
extern IrOperator op_end;

#endif /* SRC_COMPILER_OPERATOR_H */