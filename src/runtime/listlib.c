#include "object.h"
#include "gc.h"
#include "global.h"

Object *_cons(Object *one, Object *two)
{
    ConsObject *cons = (ConsObject*)gc_new(&impgc, kCons, sizeof(ConsObject));
    cons->car = one;
    cons->cdr = two;
    return (Object*)cons;
}

Object *_list(Object *rest)
{
    return rest;
}