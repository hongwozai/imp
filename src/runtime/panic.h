#ifndef SRC_RUNTIME_PANIC_H
#define SRC_RUNTIME_PANIC_H

#include <stdio.h>

#define panic(fmt, ...) {                                       \
        fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        exit(-1);                                               \
    }

#endif /* SRC_RUNTIME_PANIC_H */