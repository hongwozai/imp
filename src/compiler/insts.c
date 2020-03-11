#include <assert.h>
#include "insts.h"
#include "nodes.h"
#include "phase.h"
#include "utils/hashmap.h"

/* 指针索引node */
typedef struct TableNode {
    HashLink hlink;
    void *ptr;
    void *block;
    size_t reg;
} TableNode;

typedef HashMap Table;

static bool  table_create(Table *table, Arena *arena);
static TableNode* table_get(Table *table, void *ptr);
static void  table_set(Table *table, Arena *arena, void *ptr, Node* node);
static void  table_destroy(Table *table);

static size_t hashfunc(void *key)
{
    return (size_t)key;
}

static void *keyfunc(HashLink *link)
{
    TableNode *node = container_of(link, TableNode, hlink);
    return (void*)node->ptr;
}

static bool equalfunc(void *key, HashLink *hlink)
{
    return (size_t)key == (size_t)keyfunc(hlink);
}

static bool  table_create(Table *table, Arena *arena)
{
    if (!hashmap_arena_create(table, arena, 32,
                              hashfunc,
                              keyfunc,
                              equalfunc,
                              hashmap_extend2pow_func)) {
        return false;
    }
    return true;
}

static TableNode* table_get(Table *table, void *ptr)
{
    HashLink *hlink = hashmap_get(table, (void*)ptr);
    if (!hlink) {
        return NULL;
    }
    return container_of(hlink, TableNode, hlink);
}

static void  table_set(Table *table, Arena *arena, void *ptr, Block* block)
{
    TableNode *hnode = arena_malloc(arena, sizeof(TableNode));
    hashmap_initlink(&hnode->hlink);
    hnode->ptr = ptr;
    hnode->block = block;

    hashmap_set(table, &hnode->hlink, true);
}

static void  table_destroy(Table *table)
{
    hashmap_destroy(table);
}

/* ================================================== */
typedef struct GenInstFunc {
    ListLink link;

    /* result */
    PtrVec blockvec;

    /* temp */
    Arena funcarena;
    Table regiontab;
    Table valuetab;
    size_t curreg;
    Block *curblock;
} GenInstFunc;

typedef struct GenInstPhase {
    Phase head;

    /* result */
    Arena arena;
    List funclist;

    GenInstFunc *curfunc;
} GenInstPhase;

void geninst_func(Phase *phase, AnalyFunction *analyfunc)
{
    GenInstPhase *giphase = (GenInstPhase*)phase;
    GenInstFunc *func = arena_malloc(&giphase->arena, sizeof(GenInstFunc));
    list_link_init(&func->link);
    func->curreg = 0;

    /* free current function */
    if (giphase->curfunc) {
    }

    /* set current function */
    giphase->curfunc = func;
}

void geninst_region(Phase *phase, AnalyFunction *analyfunc, Node *node)
{
    GenInstPhase *giphase = (GenInstPhase*)phase;
    Block *block = arena_malloc(&giphase->arena, sizeof(Block));

    /* insert table */
    table_set(&giphase->regiontab, &giphase->arena, node, block);

    /* walk control parent on region */
    ptrvec_foreach(iter, &node->ctrls) {
        Node *ctrlnode = ptrvec_get(&node->ctrls, iter);
        TableNode *tabnode =
            table_get(&giphase->regiontab, ctrlnode);

        if (!tabnode) {
            assert(!"bug");
        }

        Block *parent = tabnode->block;
        ptrvec_add(&giphase->arena, &block->predecessors, parent);
        ptrvec_add(&giphase->arena, &parent->successors, block);
    }

    giphase->curblock = block;
}

void geninst_value(Phase *phase, AnalyFunction *analyfunc, Node *node)
{
    GenInstPhase *giphase = (GenInstPhase*)phase;
    Block *block = giphase->curblock;


}

GenInstPhase geninst = {
    {
        .visit_func = geninst_func,
        .visit_region = geninst_region,
        .visit_value  = geninst_value
    },
    NULL
};

void geninst_init()
{
    arena_init(&geninst.arena, 4096);
    arena_init(&geninst.temparena, 1024);
    table_create(&geninst.regiontab, &geninst.arena);
    table_create(&geninst.valuetab, &geninst.arena);

    list_init(&geninst.funclist);
    geninst.curfunc = NULL;
    geninst.curblock = NULL;
}
