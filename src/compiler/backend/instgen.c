#include "instgen.h"
#include "instgen_internal.h"
#include "instgen_x64.h"

static char *block_getname(Arena *arena)
{
    static size_t block_num = 0;

    /* 64为位足够使用，所以这里不担心其错误码 */
    char buf[64];
    size_t len = snprintf(buf, sizeof(buf), ".block%lu", block_num++);
    return arena_dup(arena, buf, len);
}

static void nodelist_push(Arena *arena, List *list, Node *node)
{
    NodeList *first = arena_malloc(arena, sizeof(NodeList));
    list_link_init(&first->link);
    first->node = node;

    list_push(list, &first->link);
}

static Block *newblock(ModuleFunc *func, Node *region)
{
    Block *block = arena_malloc(&func->arena, sizeof(Block));
    block->region = region;
    ptrvec_init(&func->arena, &block->preds, 0);
    ptrvec_init(&func->arena, &block->succs, 0);
    list_init(&block->insts);
    block->region = region;
    block->name = block_getname(&func->arena);
    list_link_init(&block->link);

    block->id = ptrvec_add(&func->arena, &func->blocks, block);
    return block;
}

static void block_addsucc(Arena *arena, Block *self, Block *succ)
{
    ptrvec_add(arena, &self->succs, succ);
}

static void block_addpred(Arena *arena, Block *self, Block *pred)
{
    ptrvec_add(arena, &self->preds, pred);
}

static ModuleFunc* newfunc(Module *module)
{
    ModuleFunc *func = arena_malloc(&module->arena, sizeof(ModuleFunc));
    list_link_init(&func->link);
    arena_init(&func->arena, 1024);
    ptrvec_init(&func->arena, &func->blocks, 1);
    func->start = NULL;
    func->vregindex = 0;
    func->stacksize = 0;
    func->alignsize = 0;
    func->calleesize = 0;
    func->calleeset = NULL;
    func->name = NULL;

    list_append(&module->funcs, &func->link);
    return func;
}

static ModuleLabel* newlabel(Arena *arena, char *name, char *data, size_t len)
{
    ModuleLabel *label = arena_malloc(arena, sizeof(ModuleLabel));
    list_link_init(&label->link);
    label->name = arena_dup(arena, name, strlen(name) + 1);
    label->data = arena_dup(arena, data, len);
    label->len = len;
    return label;
}

static void walk_node(Module *module, ModuleFunc *func, Block *block, Node *node);

static void walk_label(Module *module, ModuleFunc *func, Block *block, Node *node);

/**
 * 遍历控制流，依次生成指令
 */
static void walk_block(Module *module, ModuleFunc *func, Block *block,
                       void (*walk)(Module *module, ModuleFunc *func,
                                    Block *block, Node *node))
{
    Arena arena;
    List stack;

    arena_init(&arena, 1024);
    list_init(&stack);
    nodelist_push(&arena, &stack, block->region->attr.node);

    /* 跟随控制流，按控制流的顺序进行操作 */
    Node *curr = container_of(stack.first, NodeList, link)->node;
    while (ptrvec_count(&curr->ctrls) != 0) {
        curr = ptrvec_get(&curr->ctrls, 0);
        nodelist_push(&arena, &stack, curr);
    }

    /* 按照控制流顺序遍历 */
    list_foreach(pos, stack.first) {
        NodeList *temp = container_of(pos, NodeList, link);
        walk(module, func, block, temp->node);
    }

    arena_dispose(&arena);
}

/**
 * 该函数处理生成CFG
 */
static void walk_func(Module *module, ModuleFunc *func, AnalyFunction *analyfunc)
{
    /* 遍历所有的region */
    Node *first = analyfunc->env->curregion;

    first->next = NULL;
    first->data = newblock(func, first);

    while (first) {
        /* pop */
        Node *curr = first;
        first = first->next;

        Block *currblock = (Block*)curr->data;

        if (ptrvec_count(&curr->ctrls) == 0) {
            func->start = currblock;
        }

        ptrvec_foreach(iter, &curr->ctrls) {
            Node * parent = ptrvec_get(&curr->ctrls, iter);

            while (parent) {
                if (parent->op == kNodeRegion) {
                    Block *temp = NULL;

                    if (!parent->data) {
                        temp = newblock(func, parent);
                    } else {
                        temp = parent->data;
                    }

                    /* add succ and pred */
                    block_addsucc(&func->arena, temp, currblock);
                    block_addpred(&func->arena, currblock, temp);

                    /* push */
                    if (!parent->data) {
                        parent->next = first;
                        first = parent;
                        /* add data */
                        parent->data = temp;
                    }
                    break;
                }

                if (ptrvec_count(&parent->ctrls) == 0) {
                    break;
                } else {
                    parent = ptrvec_get(&parent->ctrls, 0);
                }
            }
        }
    }
}

void instgen_run(Analy *analy, Module *module)
{
    /* 初始化module */
    arena_init(&module->arena, 4096);
    list_init(&module->labels);
    list_init(&module->funcs);

    /* 对所有的function，先生成block */
    list_foreach(pos, analy->funclist.first) {
        AnalyFunction *analyfunc = container_of(pos, AnalyFunction, link);
        ModuleFunc *func = newfunc(module);

        if (analyfunc->name) {
            func->name = arena_dup(&func->arena,
                                   analyfunc->name, analyfunc->namelen + 1);
        }
        walk_func(module, func, analyfunc);
    }

    /* 遍历所有function的所有block */
    list_foreach(pos, module->funcs.first) {
        ModuleFunc *func = container_of(pos, ModuleFunc, link);

        ptrvec_foreach(iter, &func->blocks) {
            Block *block = ptrvec_get(&func->blocks, iter);
            /* label生成 */
            walk_block(module, func, block, walk_label);
            /* 由于代码生成时会破坏当前遍历结构，所以必须在最后 */
            walk_block(module, func, block, walk_node);
        }
    }
}

/* ========================================================= */
static bool isvisiting(Node *node) { return node->mode == kModeVisit; }

static void setvisiting(Node *node) { node->mode = kModeVisit; }

static void setmiddle(Node *node)
{
    node->mode = kModeMiddle;
}

static bool ismiddle(Node *node)
{
    return node->mode == kModeMiddle;
}

static bool isvisited(WalkMode currmode, Node *node)
{
    return node->mode == (currmode == kModeTop ? kModeBottom : kModeTop);
}

static void setvisited(WalkMode currmode, Node *node)
{
    /* 这里的访问是包括子节点也访问过 */
    node->mode = (currmode == kModeTop) ? kModeBottom : kModeTop;
}

bool instgen_isqueue(Node *node)
{
    return ismiddle(node) || isvisiting(node);
}

/**
 * 这里区分平台
 */
static void walk_node(Module *module, ModuleFunc *func, Block *block, Node *node)
{
    Node *first = node;
    WalkMode currmode = node->mode;
    InstGenOp *ops = instgenop_x64;

    if (isvisited(currmode, first)) {
        return;
    } else {
        first->next = NULL;
        setmiddle(first);
    }

    while (first) {
        Node *curr = first;

        /**
         * 这里会有一部分没有遍历到的节点没有设置状态
         */
        if (isvisiting(curr)) {
            /* need emit */
            ops[first->op].emit(func, block, curr);

            first = first->next;
            setvisited(currmode, curr);
            continue;
        } else if (isvisited(currmode, curr)) {
            first = first->next;
            continue;
        }

        assert(ismiddle(curr));

        /* 遍历获取下一步向栈中加入的元素 */
        Node *start = NULL, *last = NULL;
        ops[first->op].gen(func, curr, &start);

        for (Node *temp = start; temp != NULL; temp = temp->next) {
            setmiddle(temp);
            last = temp;
        }
        if (start) {
            last->next = first;
            first = start;
        }
        setvisiting(curr);
    }
}

static void walk_label(Module *module, ModuleFunc *func, Block *block, Node *node)
{
    Node *first = node;
    WalkMode currmode = node->mode;
    first->next = NULL;

    if (isvisited(currmode, first)) {
        return;
    }

    /* 收集使用的label */
    while (first) {
        Node *curr = first;
        first = first->next;
        setvisited(currmode, curr);

        if (curr->op == kNodeLabel &&
            curr->attr.label.datalen != 0) {
            ModuleLabel *label =
                newlabel(&module->arena,
                         curr->attr.label.name,
                         curr->attr.label.data,
                         curr->attr.label.datalen);
            list_append(&module->labels, &label->link);
            continue;
        }

        /* 控制流走过了这个就不在走控制流了 */
        ptrvec_foreach(iter, &curr->inputs) {
            Node *parent = ptrvec_get(&curr->inputs, iter);

            if (!isvisited(currmode, parent)) {
                parent->next = first;
                first = parent;
            }
        }
    }
}