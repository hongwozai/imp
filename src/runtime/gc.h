#ifndef SRC_RUNTIME_GC_H
#define SRC_RUNTIME_GC_H

#include "object.h"
#include "utils/list.h"

typedef enum MarkType {
    kWhite,
    kGrey,
    kBlack,
} MarkType;

struct LinkedMemory {
    ListLink link;
    struct LinkedMemory *greynext;
};

typedef struct GC {
    List all;
    struct LinkedMemory *grey;
} GC;

void gc_init(GC *gc);
void gc_destroy(GC *gc);

Object *gc_new(GC *gc, enum ObjectType type, size_t size);
void gc_linkgrey(GC *gc, Object *obj);
void gc_mark(GC *gc);
void gc_sweep(GC *gc);

#endif /* SRC_RUNTIME_GC_H */