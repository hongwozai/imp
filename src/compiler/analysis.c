#include "operator.h"
#include "analysis.h"
#include "runtime/panic.h"

static IrNode* visit(Analy *analy, Object *obj);
static IrNode* visit_symbol(Analy *analy, Object *obj);
static IrNode* visit_call(Analy *analy, Object *obj);
static IrNode* visit_atom(Analy *analy, Object *obj);

static AnalyEnv* newenv(AnalyFunction *func)
{
    AnalyEnv *env = arena_malloc(&func->arena, sizeof(AnalyEnv));
    env->cursymtab = arena_malloc(&func->arena, sizeof(SymTab));
    symtab_create(env->cursymtab, NULL);
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
    IrNode *region = irnode_newregion(&func->arena, 0, NULL);
    func->env->curregion = region;
    return func;
}

static void freefunc(AnalyFunction *func)
{
    /* 需要释放符号表 */
    for (SymTab *sym = func->env->cursymtab; sym != NULL; sym = sym->prev) {
        symtab_destroy(sym);
    }
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

static inline IrNode* curregion(Analy *analy)
{
    return curenv(analy)->curregion;
}

void analysis_init(Analy *analy)
{
    arena_init(&analy->arena, 1024);
    list_init(&analy->funclist);
    if (!symtab_create(&analy->consttab, NULL)) {
        panic("memory overflow!");
    }
    /* 创建toplevel的函数 */
    AnalyFunction *toplevel = newfunc(analy, NULL);
    list_append(&analy->funclist, &toplevel->link);

    analy->curfunc = toplevel;
}

void analysis_destroy(Analy *analy)
{
    /* funclist的内存在arena内，需要释放符号表相关内存 */
    list_foreach(pos, analy->funclist.first) {
        freefunc(container_of(pos, AnalyFunction, link));
    }
    arena_dispose(&analy->arena);
    symtab_destroy(&analy->consttab);
}

void analysis_analy(Analy *analy, Object *obj)
{
    visit(analy, obj);
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
    IrNode *node = irnode_newconstobj(&analy->arena, curregion(analy), obj);
    symtab_set(&analy->consttab, &analy->arena, obj, node);
    curregion(analy)->inputs[0] = node;
    return node;
}

static IrNode* visit_symbol(Analy *analy, Object *obj)
{
    IrNode *node = symtab_nestget(curenv(analy)->cursymtab,
                                  sym_getname(obj));
    if (!node) {
        /* 当前作用域找不到 */
        node = irnode_newconstobj(curarena(analy), curregion(analy), obj);
        curregion(analy)->inputs[0] = node;
        return node;
    }
    curregion(analy)->inputs[0] = node;
    return node;
}

static IrNode* visit_call(Analy *analy, Object *obj)
{
    IrNode *node = NULL;
    IrNode **nodes;

    /* 计算列表长度 */
    size_t size = cons_length(obj);

    nodes = arena_malloc(curarena(analy), sizeof(IrNode*) * size);

    /* 逐个访问，并赋值 */
    size_t i = 0;
    cons_foreach(cons, obj) {
        nodes[i] = visit(analy, cons->car);
        i++;
    }

    node = irnode_newcallobj(curarena(analy), curregion(analy),
                             curregion(analy)->inputs[1],
                             size,
                             nodes);
    curregion(analy)->inputs[0] = node;
    curregion(analy)->inputs[1] = node;
    return node;
}
