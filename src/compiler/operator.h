#ifndef SRC_COMPILER_OPERATOR_H
#define SRC_COMPILER_OPERATOR_H

#include <stdbool.h>
#include "utils/arena.h"
#include "runtime/object.h"

enum {
    /* high */
    kOpConstObj = 1,
    kOpGlobalObj,
    kOpCallObj,

    /* low */
    kOpCall,
    kOpLoadRegSize,
    kOpStoreRegSize,
    kOpAdd,
    kOpSub,
    kOpAnd,                     /* 与操作 */
    kOpShl,                     /* 左移 */
    kOpSar,                     /* 右移 */

    /* 删除结点占位符 */
    kOpDead,
};

typedef struct IrOperator {
    const char *name;
    enum {
        kValueNode,
        kControlNode,
    } type;
    bool have_attr;
} IrOperator;

extern IrOperator irallop[];

#endif /* SRC_COMPILER_OPERATOR_H */
