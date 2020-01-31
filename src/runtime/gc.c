#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gc.h"

void gc_init(GC *gc)
{
    list_init(&gc->all);
    gc->grey = NULL;
}

void gc_destroy(GC *gc)
{
    list_init(&gc->all);
    gc->grey = NULL;
}

void gc_sweep(GC *gc)
{
    assert(gc->grey);

}

Object *gc_new(GC *gc, size_t size);

void gc_markobj(GC *gc, Object *obj, MarkType mark);

void gc_linkgrey(GC *gc, Object *obj)
{
}

void gc_mark(GC *gc);
