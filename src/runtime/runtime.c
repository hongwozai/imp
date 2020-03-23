#include <stdio.h>
#include <stdarg.h>

#include "object.h"
#include "gc.h"
#include "global.h"

/**
 * 构造list
 */
Object *_runtime_list(Object *first, ...)
{
    va_list ap;
    Object *list = imp_nil;

    if (!first) { return list; }

    va_start(ap, first);
    Object *temp = first;
    Object *last = imp_nil;
    do {
        Object *cons = gc_new(&impgc, kCons, sizeof(ConsObject));
        getcar(cons) = temp;
        getcdr(cons) = imp_nil;

        if (last != imp_nil) {
            getcdr(last) = cons;
        }
        if (list == imp_nil) {
            list = cons;
        }
        last = cons;
        temp = va_arg(ap, Object*);
    } while (temp);

    va_end(ap);
    return list;
}

/**
 * 构造类型
 */
Object *_runtime_newv(enum ObjectType type, union Value value)
{
    Object *obj = gc_new(&impgc, type, sizeof(ValueObject));
    ((ValueObject*)obj)->value = value;
    return obj;
}

Object *_runtime_newstr(const char *str, size_t size)
{
    return gc_create_string(&impgc, str, size);
}
