#include <stdio.h>
#include <stdlib.h>
#include "runtime/gc.h"

int main(int argc, char *argv[])
{
    GC gc;
    gc_init(&gc);
    Object *obj = gc_new(&gc, kNil, sizeof(ValueObject));
    gc_linkgrey(&gc, obj);
    gc_mark(&gc);
    gc_sweep(&gc);
    if (gc.all.size == 0) {
        exit(-1);
    }
    /* gc_linkgrey(&gc, obj); */
    gc_mark(&gc);
    gc_sweep(&gc);
    if (gc.all.size != 0) {
        exit(-1);
    }
    return 0;
}