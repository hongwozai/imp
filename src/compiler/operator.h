#ifndef SRC_COMPILER_OPERATOR_H
#define SRC_COMPILER_OPERATOR_H

#include <stdbool.h>
#include "utils/arena.h"
#include "runtime/object.h"

typedef enum IrOpcode {
    /* high */
    kOpConstObj = 1,
    kOpCallObj,
    kOpIfObj,

    /* control flow */
    kOpRegion,
    kOpIf,

    /* low */
    kOpConstLabel,
    kOpImm,
    kOpCall,
    kOpLoad,
    kOpStore,
    kOpAdd,
    kOpSub,
    kOpNot,
    kOpAnd,                     /* 与操作 */
    kOpShl,                     /* 左移 */
    kOpSar,                     /* 右移 */

    /* 删除结点占位符 */
    kOpDead,
} IrOpcode;

typedef struct IrOperator {
    const char *name;
} IrOperator;

typedef union IrAttr {
    /* constobj */
    struct ObjAttr {
        Object *obj;
    } objattr;
    struct LabelAttr {
        char *label;            /* \0结尾 */
        char *content;
        size_t size;            /* content的长度 */
    } labelattr;
} IrAttr;

extern IrOperator irallop[];

#endif /* SRC_COMPILER_OPERATOR_H */
