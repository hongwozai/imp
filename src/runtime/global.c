#include "global.h"

GC impgc;

/* bool */
Object *imp_true;
Object *imp_false;

/* nil */
Object *imp_nil;

/* symbol */
Object *imp_define;
Object *imp_set;
Object *imp_lambda;
Object *imp_if;

void global_init()
{
    gc_init(&impgc);

    imp_true = gc_new(&impgc, kBool, sizeof(ValueObject));
    getvalue(imp_true)._bool = true;

    imp_false = gc_new(&impgc, kBool, sizeof(ValueObject));
    getvalue(imp_false)._bool = false;

    imp_nil = gc_new(&impgc, kNil, sizeof(ValueObject));

    /* symbol */
    imp_define = gc_create_symbol(&impgc, "define", 6, true);
    imp_set = gc_create_symbol(&impgc, "set!", 4, true);
    imp_lambda = gc_create_symbol(&impgc, "lambda", 6, true);
    imp_if = gc_create_symbol(&impgc, "if", 2, true);
}

void global_destroy()
{
    gc_destroy(&impgc);
}
