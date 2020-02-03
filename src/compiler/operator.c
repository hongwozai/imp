#include "operator.h"

IrOperator irallop[] = {
    /* high */
    [kOpConstObj] = {
        .name        = "constobj",
        .type        = kValueNode,
        .have_attr   = true,
    },
    [kOpGlobalObj] = {
        .name        = "globalobj",
        .type        = kValueNode,
        .have_attr   = true,
    },
    [kOpCallObj] = {
        .name        = "callobj",
        .type        = kValueNode,
        .have_attr   = false,
    },
    /* low */
    [kOpCall] = {
        .name        = "call",
        .type        = kValueNode,
        .have_attr   = false,
    },
    [kOpLoadRegSize] = {
        .name        = "loadregsize",
        .type        = kValueNode,
        .have_attr   = false,
    },
    [kOpStoreRegSize] = {
        .name        = "storeregsize",
        .type        = kValueNode,
        .have_attr   = false,
    },
    [kOpAdd] = {
        .name      = "add",
        .type      = kValueNode,
        .have_attr = false,
    },
    [kOpSub] = {
        .name      = "sub",
        .type      = kValueNode,
        .have_attr = false,
    },
    [kOpAnd] = {
        .name      = "and",
        .type      = kValueNode,
        .have_attr = false,
    },
    [kOpShl] = {
        .name      = "shl",
        .type      = kValueNode,
        .have_attr = false,
    },
    [kOpSar] = {
        .name      = "sar",
        .type      = kValueNode,
        .have_attr = false,
    },
    /* dead */
    [kOpDead] = {
        .name        = "dead",
        .type        = kControlNode,
        .have_attr   = false,
    },
};
