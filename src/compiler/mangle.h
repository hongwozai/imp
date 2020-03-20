#ifndef IMP_SRC_COMPILER_MANGLE_H
#define IMP_SRC_COMPILER_MANGLE_H

#include "utils/arena.h"

/**
 * mangle规则
 * 除了数字和字符串，其他都使用二进制0xFF这类方式进行
 * _[namespace]_[name]
 */
char *mangle(Arena *arena, char *name);

#endif /* IMP_SRC_COMPILER_MANGLE_H */