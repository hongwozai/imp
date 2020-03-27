#include "nodes.h"
#include "analysis.h"
#include "runtime/panic.h"
#include "runtime/global.h"

static Node* visit(Analy *analy, Object *obj);
static Node* visit_symbol(Analy *analy, Object *obj);
static Node* visit_call(Analy *analy, Object *obj);
static Node* visit_atom(Analy *analy, Object *obj);
static Node* visit_define(Analy *analy, Object *obj);
static Node* visit_define_var(Analy *analy, Object *obj);
static Node* visit_define_lambda(Analy *analy, Object *obj);
static Node* visit_lambda(Analy *analy, Object *args, Object *body,
                          char *name, size_t namelen);
static Node* visit_do(Analy *analy, Object *obj);

/**
 * 获取临时的名称
 */
static char *func_getname(Arena *arena)
{
    static size_t func_num = 0;

    /* 64为位足够使用，所以这里不担心其错误码 */
    char buf[64];
    size_t len = snprintf(buf, sizeof(buf), ".func%lu", func_num++);
    buf[len] = '\0';
    return arena_dup(arena, buf, len + 1);
}

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

    /* special */
    if (gettype(getcar(obj)) == kSymbol) {
        if (equal_object(getcar(obj), imp_define)) {
            return visit_define(analy, getcdr(obj));
        } else if (equal_object(getcar(obj), imp_lambda)) {
            Object *rest = getcdr(obj);
            char *name = func_getname(curarena(analy));
            size_t namelen = strlen(name);

            assert(gettype(rest) == kCons);
            assert(gettype(getcar(rest)) == kCons ||
                   gettype(getcar(rest)) == kNil);
            return visit_lambda(analy, getcar(rest), getcdr(rest),
                                name, namelen);
        }
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

    /* 全局变量如果找不到，那就是内建符号，如果还找不到，那么就失败 */
    return node;
}

static Node* visit_define(Analy *analy, Object *obj)
{
    if (gettype(getcar(obj)) == kCons) {
        /* (define (x a b c) <xxx>) */
        return visit_define_lambda(analy, obj);
    }
    /* (define x <xx> ) */
    /* assert(!"[syntax error] definevar not symbol"); */
    return visit_define_var(analy, obj);
}

static Node* visit_define_var(Analy *analy, Object *obj)
{
    Object *sym, *value;

    if (gettype(getcar(obj)) != kSymbol) {
        assert(!"[syntax error] define var syntax error");
    }

    sym = getcar(obj);
    value = (gettype(getcdr(obj)) != kCons)
        ? imp_nil
        : getcar(getcdr(obj));

    /* 访问value */
    Node *node = visit(analy, value);

    /* 添加符号 */
    symtab_set(curenv(analy)->cursymtab, curarena(analy),
               sym_getname(sym), node);

    return node;
}

static Node* visit_define_lambda(Analy *analy, Object *obj)
{
    /* name and args */
    Object *na = getcar(obj);
    Object *body = getcdr(obj);

    assert(gettype(na) == kCons && gettype(getcar(na)) == kSymbol);

    /* 获得funcname */
    char *name = str_getcstr(sym_getname(getcar(na)));
    size_t namelen = str_getsize(sym_getname(getcar(na)));

    /* 这里构造funcobj */
    Node *func = visit_lambda(analy, getcdr(na), body, name, namelen);

    /* 添加符号 */
    symtab_set(curenv(analy)->cursymtab, curarena(analy),
               sym_getname(getcar(na)), func);

    return func;
}

static Node* visit_lambda(Analy *analy, Object *args, Object *body,
                          char *name, size_t namelen)
{
    AnalyFunction *func = newfunc(analy, analy->curfunc);
    func->name = name;
    func->namelen = namelen;

    list_append(&analy->funclist, &func->link);

    /* 对body进行求值 */
    analy->curfunc = func;
    {
        Node *retval = visit_do(analy, body);
        Node *retnode = node_new(curarena(analy), kNodeReturn);
        node_addinput(curarena(analy), retnode, retval, false);

        /* 将retnode加入region记录 */
        if (curregion(analy)->attr.node) {
            node_addinput(curarena(analy), retnode,
                          curregion(analy)->attr.node, true);
        }
        curregion(analy)->attr.node = retnode;

    }
    analy->curfunc = func->parent;

    /* 构造函数对象 */
    Node *label = node_newlabel(curarena(analy), name, 0, 0);
    Node *type  = node_new(curarena(analy), kNodeImm);
    type->attr.imm = kPrimFunc;
    Node *funclabel = node_newlabel(curarena(analy),
                                    arena_dup(curarena(analy), "_runtime_newv",
                                              sizeof("_runtime_newv")),
                                    0,
                                    0);
    Node *call  = node_new(curarena(analy), kNodeCall);
    node_addinput(curarena(analy), call, funclabel, false);
    node_addinput(curarena(analy), call, type, false);
    node_addinput(curarena(analy), call, label, false);
    return call;
}

static Node* visit_do(Analy *analy, Object *obj)
{
    Node *last = NULL;
    cons_foreach(cons, obj) {
        last = visit(analy, cons->car);
    }
    return last;
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
    return callnode;
}
