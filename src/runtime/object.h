#ifndef SRC_RUNTIME_OBJECT_H
#define SRC_RUNTIME_OBJECT_H

#include <stdint.h>
#include <stddef.h>

/**
 * @ObjectType
 * 值类型：可以使用uintptr_t来表示的类型
 *        可以对值类型进行类型推导，并进行标量替换
 *        并可以在编译时，直接使用汇编进行操作
 * 对象类型： 对象类型的操作使用c函数进行
 *          对对象类型不进行标量替换操作
 */

/**
 * 具体类型值
 */
enum ObjectType {
    /* 值类型排在前面 */
    kFixInt,
    kFixFloat,
    kBool,
    kChar,
    kEof,
    kNil,
    VALUE_TYPE_MAX,

    /* 对象类型排在后面 */
    kString,
    kCons,
    kSymbol,
};

/**
 * 原始对象结构
 * 32b - 3B 类型 低1B mark标记
 * 64b - 7B 类型 低1B mark标记
 * 获取类型的方法：  1. & ~0xFF 2. >> 8
 * 获取mark的方法： 1. & 0xFF
 */
typedef struct Object {
    uintptr_t marked_type;
} Object;

#define gettype(obj) ((((Object*)obj)->marked_type & ~0xFF) >> 8)
#define getmark(obj) (((Object*)obj)->marked_type & 0xFF)

#define settype(obj, type)                                      \
    (((Object*)obj)->marked_type = getmark(obj) | (type) << 8)
#define setmark(obj, mark)                                              \
    (((Object*)obj)->marked_type = (((Object*)obj)->marked_type & ~0xFF) | (mark))

/**
 * 值类型
 */
typedef union Value {
    intptr_t fixint;
    double   fixfloat;
    uintptr_t _bool;
    uintptr_t _char;
    /* eof, nil无值，用类型进行区分 */
} Value;

typedef struct ValueObject {
    Object head;
    Value value;
} ValueObject;

#define getvalue(obj) (((ValueObject*)(obj))->value)

/**
 * 对象类型
 */
#define STRING_OBJECT_STATIC_SIZE (16)
typedef struct StringObject {
    Object head;
    size_t size;
    /* 本地存储，默认存储 */
    char buf[STRING_OBJECT_STATIC_SIZE];
} StringObject;

typedef struct ConsObject {
    Object head;
    Object *car;
    Object *cdr;
} ConsObject;

#define getcar(xobj) (((ConsObject*)xobj)->car)
#define getcdr(xobj) (((ConsObject*)xobj)->cdr)
#define cons_foreach(cons, head)                \
    for (ConsObject *cons = (ConsObject*)head;  \
         gettype(cons) == kCons;                \
         cons = (ConsObject*)getcdr(cons))

typedef struct SymbolObject {
    Object head;
    Object *name;
} SymbolObject;

#define sym_getname(obj) (((SymbolObject*)obj)->name)

#include <stdio.h>
#include <stdbool.h>

void print_object(FILE *out, Object *obj);
bool equal_object(Object *one, Object *two);

#endif /* SRC_RUNTIME_OBJECT_H */
