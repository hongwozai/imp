#include "object.h"
#include "gc.h"
#include "global.h"

Object *_print0x2Dobject(Object *obj)
{
    print_object(stdout, obj);
    return imp_nil;
}