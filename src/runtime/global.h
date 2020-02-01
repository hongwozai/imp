#ifndef SRC_RUNTIME_GLOBAL_H
#define SRC_RUNTIME_GLOBAL_H

#include "gc.h"

/**
 * 语言本身的全局变量
 */
extern GC impgc;

/* bool */
extern Object *imp_true;
extern Object *imp_false;

/* nil */
extern Object *imp_nil;

/* special symbol */
extern Object *imp_define;
extern Object *imp_set;
extern Object *imp_lambda;
extern Object *imp_if;

void global_init();

#endif /* SRC_RUNTIME_GLOBAL_H */