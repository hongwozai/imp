#include "object.h"
#include "panic.h"

void print_object(FILE *out, Object *obj)
{
    switch (gettype(obj)) {
    case kFixInt: {
        fprintf(out, "%ld", getvalue(obj).fixint);
        break;
    }
    case kFixFloat: {
        fprintf(out, "%lf", getvalue(obj).fixfloat);
        break;
    }
    case kBool: {
        fprintf(out, "%s", getvalue(obj)._bool ? "true" : "false");
        break;
    }
    case kChar: {
        fprintf(out, "%c", (char)getvalue(obj)._char);
        break;
    }
    case kEof: {
        fprintf(out, "#<EOF>");
        break;
    }
    case kNil: {
        fprintf(out, "nil");
        break;
    }
    case kString: {
        StringObject *str = (StringObject*)obj;
        fprintf(out, "\"%.*s\"", (int)str->size, str->buf);
        break;
    }
    case kSymbol: {
        StringObject *name = (StringObject*)((SymbolObject*)obj)->name;
        fprintf(out, "%.*s", (int)name->size, name->buf);
        break;
    }
    case kCons: {
        fprintf(out, "(");
        cons_foreach(cons, obj) {
            print_object(out, cons->car);
            if (gettype(cons->cdr) == kCons) {
                fprintf(out, " ");
            }
        }
        fprintf(out, ")");
        break;
    }
    default:
        panic("bug! check code!");
    }
}