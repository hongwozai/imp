#ifndef IMP_SRC_RUNTIME_PANIC_H
#define IMP_SRC_RUNTIME_PANIC_H

#include <stdio.h>
#include <stdlib.h>

#define panic(fmt, ...) {                                               \
        fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        exit(-1);                                                       \
    }

#endif /* IMP_SRC_RUNTIME_PANIC_H */