#ifndef SRC_COMPILER_PTRVEC_H
#define SRC_COMPILER_PTRVEC_H

#include <stddef.h>

typedef struct PtrVec {
    void **vecs;
    size_t count;
} PtrVec;

void ptrvec_init(Arena *arena, PtrVec *vec, size_t size);
void ptrvec_add(Arena *arena, PtrVec *vec, void *obj);

#endif /* SRC_COMPILER_PTRVEC_H */