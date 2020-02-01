#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gc.h"

#define getobj(mem) (Object*)((struct LinkedMemory*)(mem) + 1)

void gc_init(GC *gc)
{
    list_init(&gc->all);
    gc->grey = NULL;
}

void gc_destroy(GC *gc)
{
    list_safe_foreach(temp, iter, gc->all.first) {
        struct LinkedMemory *mem =
            container_of(temp, struct LinkedMemory, link);
        list_del(&gc->all, temp);
        free(mem);
    }

    list_init(&gc->all);
    gc->grey = NULL;
}

void gc_sweep(GC *gc)
{
    assert(gc->grey == NULL);

    list_safe_foreach(temp, iter, gc->all.first) {
        Object *obj =
            getobj(container_of(temp, struct LinkedMemory, link));

        if (getmark(obj) == kWhite) {
            list_del(&gc->all, temp);
            free(temp);
            continue;
        } else {
            setmark(obj, kWhite);
        }
    }
}

Object *gc_new(GC *gc, enum ObjectType type, size_t size)
{
    struct LinkedMemory *mem =
        (struct LinkedMemory*)malloc(size + sizeof(struct LinkedMemory));
    Object *obj = getobj(mem);

    if (!mem) {
        return NULL;
    }
    mem->greynext = NULL;
    list_append(&gc->all, &mem->link);

    settype(obj, type);
    setmark(obj, kWhite);
    return obj;
}

void gc_linkgrey(GC *gc, Object *obj)
{
    struct LinkedMemory *mem = (struct LinkedMemory*)obj - 1;
    mem->greynext = gc->grey;
    gc->grey = mem;

    setmark(obj, kGrey);
}

static void markchildobj(Object *obj);

void gc_mark(GC *gc)
{
    while (gc->grey) {
        struct LinkedMemory *mem = gc->grey;

        /* mark all child object */
        markchildobj(getobj(mem));
        setmark(getobj(gc->grey), kBlack);

        /* next */
        gc->grey = gc->grey->greynext;
    }
}

void markchildobj(Object *obj)
{
    switch (gettype(obj)) {
    case kCons: {
        ConsObject *cons = (ConsObject*)obj;
        setmark(cons->car, kGrey);
        setmark(cons->cdr, kGrey);
        break;
    }
    }
}