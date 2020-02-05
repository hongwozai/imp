#include "operator.h"

IrOperator irallop[] = {
    /* high */
    [kOpConstObj] = {
        .name        = "constobj",
    },
    [kOpCallObj] = {
        .name        = "callobj",
    },
    [kOpIfObj] = {
        .name        = "ifobj",
    },
    [kOpRegion] = {
        .name        = "region",
    },
    [kOpIf] = {
        .name        = "if",
    },
    /* low */
    [kOpCall] = {
        .name        = "call",
    },
    [kOpLoad] = {
        .name        = "load",
    },
    [kOpStore] = {
        .name        = "store",
    },
    [kOpAdd] = {
        .name      = "add",
    },
    [kOpSub] = {
        .name      = "sub",
    },
    [kOpAnd] = {
        .name      = "and",
    },
    [kOpShl] = {
        .name      = "shl",
    },
    [kOpSar] = {
        .name      = "sar",
    },
    /* dead */
    [kOpDead] = {
        .name        = "dead",
    },
};
