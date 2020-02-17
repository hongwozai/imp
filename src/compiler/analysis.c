#include "nodes.h"
#include "analysis.h"
#include "runtime/panic.h"

static Node* visit(Analy *analy, Object *obj);
static Node* visit_symbol(Analy *analy, Object *obj);
static Node* visit_call(Analy *analy, Object *obj);
static Node* visit_atom(Analy *analy, Object *obj);

static AnalyEnv* newenv(AnalyFunction *func)
{
    AnalyEnv *env = arena_malloc(&func->arena, sizeof(AnalyEnv));
    env->cursymtab = arena_malloc(&func->arena, sizeof(SymTab));
    symtab_create(env->cursymtab, &func->arena, NULL);
    env->curregion = NULL;
    return env;
}

static AnalyFunction* newfunc(Analy *analy, AnalyFunction *parent)
{
    AnalyFunction *func =
        arena_malloc(&analy->arena, sizeof(AnalyFunction));

    list_link_init(&func->link);
    arena_init(&func->arena, 4096);
    func->env = newenv(func);
    func->parent = parent;

    /* 新创建一个region */
    Node *region = node_new(&func->arena, kNodeRegion);
    region->attr.node = NULL;
    func->env->curregion = region;
    return func;
}

static void freefunc(AnalyFunction *func)
{
    arena_dispose(&func->arena);
}

static inline AnalyEnv* curenv(Analy *analy)
{
    return analy->curfunc->env;
}

static inline Arena* curarena(Analy *analy)
{
    return &analy->curfunc->arena;
}

static inline Node* curregion(Analy *analy)
{
    return curenv(analy)->curregion;
}

void analysis_init(Analy *analy)
{
    arena_init(&analy->arena, 1024);
    list_init(&analy->funclist);
    if (!symtab_create(&analy->consttab, &analy->arena, NULL)) {
        panic("memory overflow!");
    }
    /* 创建toplevel的函数 */
    AnalyFunction *toplevel = newfunc(analy, NULL);
    list_append(&analy->funclist, &toplevel->link);

    analy->curfunc = toplevel;
}

void analysis_destroy(Analy *analy)
{
    /* funclist的内存在arena内 */
    list_foreach(pos, analy->funclist.first) {
        freefunc(container_of(pos, AnalyFunction, link));
    }
    symtab_destroy(&analy->consttab);
    arena_dispose(&analy->arena);
}

void analysis_analy(Analy *analy, Object *obj)
{
    visit(analy, obj);
}

/**
 * 访问者模式
 */
static Node *visit(Analy *analy, Object *obj)
{
    if (gettype(obj) == kSymbol) {
        return visit_symbol(analy, obj);
    }

    if (gettype(obj) != kCons) {
        return visit_atom(analy, obj);
    }

    return visit_call(analy, obj);
}

static Node* visit_atom(Analy *analy, Object *obj)
{
    Node *node;

    node = symtab_get(&analy->consttab, obj);
    /* 常量唯一 */
    if (node) {
        return node;
    }

    /* 常量使用全局的arena */
    node = node_new(&analy->arena, kNodeConstObj);
    node->attr.obj = obj;

    symtab_set(&analy->consttab, &analy->arena, obj, node);
    return node;
}

static Node* visit_symbol(Analy *analy, Object *obj)
{
    Node *node = symtab_nestget(curenv(analy)->cursymtab, sym_getname(obj));
    if (node) {
        /* 当前作用域找到 */
        return node;
    }

    /* 当前作用域找不到，认为是全局变量 */
    node = node_new(curarena(analy), kNodeGlobalObj);
    node->attr.obj = obj;
    return node;
}

static Node* visit_call(Analy *analy, Object *obj)
{
    Node *node = NULL;
    Node *callnode = node_new(curarena(analy), kNodeCallObj);

    cons_foreach(cons, obj) {
        node = visit(analy, cons->car);
        node_addinput(curarena(analy), callnode, node, false);
    }

    if (curregion(analy)->attr.node) {
        node_addinput(curarena(analy), callnode,
                      curregion(analy)->attr.node, true);
    }
    curregion(analy)->attr.node = callnode;
    return node;
}
