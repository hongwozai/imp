#include "operator.h"
#include "analysis.h"
#include "runtime/panic.h"

static IrNode* visit(Analy *analy, Object *obj);
static IrNode* visit_symbol(Analy *analy, Object *obj);
static IrNode* visit_call(Analy *analy, Object *obj);
static IrNode* visit_atom(Analy *analy, Object *obj);

static IrNode* newnode(Arena *arena, IrOperator *op)
{
    IrNode *node = (IrNode*)arena_malloc(arena, sizeof(IrNode));
    node->op = op;
    node->values = NULL;
    node->controls = NULL;
    node->value_count = 0;
    node->control_count = 0;
    return node;
}

static AnalyFunction* newfunc(Analy *analy)
{
    AnalyFunction *func =
        arena_malloc(&analy->arena, sizeof(AnalyFunction));
    list_link_init(&func->link);
    arena_init(&func->arena, 4096);

    func->cursymtab = (SymTab*)arena_malloc(&func->arena, sizeof(SymTab));
    symtab_create(func->cursymtab, NULL);
    /* ir graph */
    return func;
}

static AnalyFunction *curfunc(Analy *analy)
{
    return container_of(analy->funclist.first, AnalyFunction, link);
}

void analysis_init(Analy *analy)
{
    arena_init(&analy->arena, 1024);
    list_init(&analy->funclist);
    if (!symtab_create(&analy->consttab, NULL)) {
        panic("memory overflow!");
    }
    /* 创建toplevel的函数 */
    AnalyFunction *toplevel = newfunc(analy);
    list_append(&analy->funclist, &toplevel->link);
}

void analysis_destroy(Analy *analy)
{
    arena_dispose(&analy->arena);
    symtab_destroy(&analy->consttab);
    /* funclist的内存在arena内 */
}

IrNode *analysis_analy(Analy *analy, Object *obj)
{
    return visit(analy, obj);
}

/**
 * 访问者模式
 */
static IrNode *visit(Analy *analy, Object *obj)
{
    if (gettype(obj) == kSymbol) {
        return visit_symbol(analy, obj);
    }

    if (gettype(obj) != kCons) {
        return visit_atom(analy, obj);
    }

    return visit_call(analy, obj);
}

static IrNode* visit_atom(Analy *analy, Object *obj)
{
    /* 使用全局的arena */
    IrNode *node = newnode(&analy->arena, &op_constobj);
    symtab_set(&analy->consttab, &analy->arena, obj, node);
    return node;
}

static IrNode* visit_symbol(Analy *analy, Object *obj)
{
    IrNode *node = symtab_get(curfunc(analy)->cursymtab, sym_getname(obj));
    if (!node) {
        /* 当前作用域找不到 */
        node = newnode(&curfunc(analy)->arena, &op_globalobj);
        return node;
    }
    return node;
}

static IrNode* visit_call(Analy *analy, Object *obj)
{
    IrNode *node = NULL;
    IrNode **nodes;

    /* 计算列表长度 */
    size_t size = 0;
    cons_foreach(cons, obj) {
        size ++;
    }

    nodes = arena_malloc(&curfunc(analy)->arena, sizeof(IrNode*) * size);

    /* 逐个访问，并赋值 */
    size_t i = 0;
    cons_foreach(cons, obj) {
        nodes[i] = visit(analy, cons->car);
        i++;
    }

    node = newnode(&curfunc(analy)->arena, &op_callobj);
    node->values = nodes;
    return node;
}
