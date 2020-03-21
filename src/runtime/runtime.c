#include <stdio.h>
#include "object.h"
#include "gc.h"
#include "global.h"

Object *_0x2D(Object *obj1, Object *obj2)
{
    printf("+++++++++++ %lf\n", 1.0);
    return obj1;
}

Object *_0x2B(Object *obj1, Object *obj2)
{
    printf("------------\n");
    return obj1;
}

Object *_print0x2Dobject(Object *obj)
{
    print_object(stdout, obj);
    return imp_nil;
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

/**
 * main函数入口
 */
extern intptr_t _main();

int main(int argc, char *argv[])
{
    intptr_t ret = 0;
    global_init();
    ret = _main();
    global_destroy();
    return ret;
}