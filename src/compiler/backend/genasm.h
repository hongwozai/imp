#ifndef IMP_SRC_COMPILER_BACKEND_GENASM_H
#define IMP_SRC_COMPILER_BACKEND_GENASM_H

#include <stdio.h>
#include "insts.h"

void genasm_run(FILE *out, Module *module);

#endif /* IMP_SRC_COMPILER_BACKEND_GENASM_H */