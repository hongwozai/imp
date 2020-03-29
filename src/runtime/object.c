#include <stdlib.h>
#include <string.h>
#include "object.h"
#include "panic.h"
#include "global.h"

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
        Object *lastcdr = imp_nil;

        fprintf(out, "(");
        cons_foreach(cons, obj) {
            print_object(out, cons->car);
            if (gettype(cons->cdr) == kCons) {
                fprintf(out, " ");
            }
            lastcdr = cons->cdr;
        }
        if (gettype(lastcdr) != kNil) {
            fprintf(out, " . ");
            print_object(out, lastcdr);
        }
        fprintf(out, ")");
        break;
    }
    default:
        panic("bug! check code!");
    }
}

bool equal_object(Object *one, Object *two)
{
    if (gettype(one) != gettype(two)) {
        return false;
    }

    switch (gettype(one)) {
    case kFixInt: {
        return getvalue(one).fixint == getvalue(two).fixint;
    }
    case kFixFloat: {
        return getvalue(one).fixfloat == getvalue(two).fixfloat;
    }
    case kBool: {
        return getvalue(one)._bool == getvalue(two)._bool;
    }
    case kChar: {
        return getvalue(one)._char == getvalue(two)._char;
    }
    case kEof: return true;
    case kNil: return true;

    case kString: {
        StringObject *onestr = (StringObject*)one;
        StringObject *twostr = (StringObject*)two;
        if (onestr->size != twostr->size) {
            return false;
        }
        return strncmp(onestr->buf, twostr->buf, onestr->size) == 0;
    }
    case kSymbol: {
        return equal_object(
            ((SymbolObject*)one)->name,
            ((SymbolObject*)two)->name);
    }
    case kCons: {
        return one == two;
    }
    default:
        panic("bug! check code!");
    }
    return false;
}