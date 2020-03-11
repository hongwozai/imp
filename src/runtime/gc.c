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
        } else if (getmark(obj) != kFix) {
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

Object *gc_vnew(GC *gc, enum ObjectType type, union Value value)
{
    Object *obj = gc_new(gc, type, sizeof(ValueObject));
    ((ValueObject*)obj)->value = value;
    return obj;
}

void gc_linkgrey(GC *gc, Object *obj)
{
    if (getmark(obj) == kGrey) {
        return;
    }

    struct LinkedMemory *mem = (struct LinkedMemory*)obj - 1;
    mem->greynext = gc->grey;
    gc->grey = mem;

    setmark(obj, kGrey);
}

static void markchildobj(GC *gc, Object *obj);

void gc_mark(GC *gc)
{
    while (gc->grey) {
        struct LinkedMemory *mem = gc->grey;

        /* mark all child object */
        markchildobj(gc, getobj(mem));
        setmark(getobj(gc->grey), kBlack);

        /* next */
        gc->grey = gc->grey->greynext;
    }
}

void markchildobj(GC *gc, Object *obj)
{
    switch (gettype(obj)) {
    case kCons: {
        ConsObject *cons = (ConsObject*)obj;
        gc_linkgrey(gc, cons->car);
        gc_linkgrey(gc, cons->cdr);
        break;
    }
    case kSymbol: {
        SymbolObject *sym = (SymbolObject*)obj;
        gc_linkgrey(gc, sym->name);
        break;
    }
    }
}

Object* gc_create_string(GC *gc, const char *str, size_t size)
{
    size_t newsize = (size <= STRING_OBJECT_STATIC_SIZE - 1)
        ? sizeof(StringObject)
        : sizeof(StringObject) + size - STRING_OBJECT_STATIC_SIZE + 1;
    StringObject *name = (StringObject*)gc_new(gc, kString, newsize);

    /* copy string buf */
    memcpy(name->buf, str, size);
    name->size = size;
    name->buf[name->size] = 0;
    return (Object*)name;
}

Object* gc_create_symbol(GC *gc, const char *str, size_t size, bool isfix)
{
    StringObject *name = (StringObject*)gc_create_string(gc, str, size);
    SymbolObject *sym = (SymbolObject*)gc_new(gc, kSymbol, sizeof(SymbolObject));

    /* symbol */
    sym->name = (Object*)name;

    if (isfix) {
        setmark((Object*)name, kFix);
        setmark((Object*)sym, kFix);
    }
    return (Object*)sym;
}
