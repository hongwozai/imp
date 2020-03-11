#ifndef SRC_RUNTIME_GC_H
#define SRC_RUNTIME_GC_H

#include "object.h"
#include "utils/list.h"

typedef enum MarkType {
    kWhite,
    kGrey,
    kBlack,
    kFix,
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
Object *gc_vnew(GC *gc, enum ObjectType type, Value value);

void gc_linkgrey(GC *gc, Object *obj);
void gc_mark(GC *gc);
void gc_sweep(GC *gc);

Object* gc_create_string(GC *gc, const char *str, size_t size);
Object* gc_create_symbol(GC *gc, const char *str, size_t size, bool isfix);

#endif /* SRC_RUNTIME_GC_H */