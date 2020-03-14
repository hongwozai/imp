#include "instgen.h"

static char *block_getname(Arena *arena)
{
    static size_t block_num = 0;

    /* 64为位足够使用，所以这里不担心其错误码 */
    char buf[64];
    size_t len = snprintf(buf, sizeof(buf), ".block%lu", block_num++);
    return arena_dup(arena, buf, len);
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

    ptrvec_add(&func->arena, &func->blocks, block);
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

    list_append(&module->funcs, &func->link);
    return func;
}

static ModuleLabel* newlabel(Arena *arena, char *name, char *data, char *len)
{
    return NULL;
}

extern void instgen_x64(ModuleFunc *func, Block *block, Node *node);
static void instgen(ModuleFunc *func, Block *block, Node *node)
{
    assert(!node->data);
    node_dprint(stdout, node);
    fprintf(stdout, "\n");
    instgen_x64(func, block, node);
}

static void walk_block(Module *module, ModuleFunc *func, Block *block)
{
    Node *first = block->region->attr.node;
    first->next = NULL;

    /* 跟随控制流，按控制流的顺序进行操作 */
    while (ptrvec_count(&first->ctrls) != 0) {
        Node *temp = ptrvec_get(&first->ctrls, 0);
        temp->next = first;
        first = temp;
    }

    for (Node *temp = first; temp != NULL; temp = temp->next) {
        instgen(func, block, temp);
    }
}

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

        walk_func(module, func, analyfunc);
    }

    /* 遍历所有function的所有block */
    list_foreach(pos, module->funcs.first) {
        ModuleFunc *func = container_of(pos, ModuleFunc, link);

        ptrvec_foreach(iter, &func->blocks) {
            Block *block = ptrvec_get(&func->blocks, iter);
            walk_block(module, func, block);
        }
    }
}