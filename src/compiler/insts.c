#include <assert.h>
#include "insts.h"
#include "nodes.h"
#include "phase.h"
#include "utils/hashmap.h"

/* 指针索引node */
typedef struct TableNode {
    HashLink hlink;
    void *ptr;
    /* value */
    void *block;
    size_t reg;
} TableNode;

typedef HashMap Table;

static bool  table_create(Table *table, Arena *arena);
static TableNode* table_get(Table *table, void *ptr);
static void  table_set(Table *table, Arena *arena, void *ptr, Block* block);
static void  table_setreg(Table *table, Arena *arena, void *ptr, size_t reg);
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

static void  table_setreg(Table *table, Arena *arena, void *ptr, size_t reg)
{
    TableNode *hnode = arena_malloc(arena, sizeof(TableNode));
    hashmap_initlink(&hnode->hlink);
    hnode->ptr = ptr;
    hnode->reg = reg;

    hashmap_set(table, &hnode->hlink, true);
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

    size_t regindex;
    Block *curblock;
} GenInstFunc;

typedef struct GenInstLabel {
    ListLink link;

    char *name;
    char *data;
    size_t datalen;
} GenInstLabel;

typedef struct GenInstPhase {
    Phase head;

    Arena arena;

    /* result */
    List funclist;
    GenInstFunc *curfunc;

    /* global data */
    List labellist;
} GenInstPhase;

void geninst_func(Phase *phase, AnalyFunction *analyfunc)
{
    GenInstPhase *giphase = (GenInstPhase*)phase;

    /* contruct new function */
    GenInstFunc *func = arena_malloc(&giphase->arena, sizeof(GenInstFunc));
    list_link_init(&func->link);
    ptrvec_init(&giphase->arena, &func->blockvec, 4);
    arena_init(&func->funcarena, 1024);
    table_create(&func->regiontab, &func->funcarena);
    table_create(&func->valuetab, &func->funcarena);
    func->regindex = 0;
    func->curblock = NULL;

    list_append(&giphase->funclist, &func->link);

    /* free temp data in current function */
    if (giphase->curfunc) {
        table_destroy(&giphase->curfunc->regiontab);
        table_destroy(&giphase->curfunc->valuetab);
        arena_dispose(&giphase->curfunc->funcarena);
    }

    /* set current function */
    giphase->curfunc = func;
}

void geninst_region(Phase *phase, AnalyFunction *analyfunc, Node *node)
{
    GenInstPhase *giphase = (GenInstPhase*)phase;

    /* new block */
    Block *block = arena_malloc(&giphase->arena, sizeof(Block));
    ptrvec_init(&giphase->arena, &block->preds, 2);
    ptrvec_init(&giphase->arena, &block->succs, 2);
    list_init(&block->insts);
    ptrvec_add(&giphase->arena, &giphase->curfunc->blockvec, block);

    /* insert table */
    table_set(&giphase->curfunc->regiontab, &giphase->curfunc->funcarena, node, block);

    /* walk control parent on region */
    ptrvec_foreach(iter, &node->ctrls) {
        Node *ctrlnode = ptrvec_get(&node->ctrls, iter);
        TableNode *tabnode =
            table_get(&giphase->curfunc->regiontab, ctrlnode);

        assert(tabnode || !"bug");

        /* every block */
        Block *parent = (Block*)tabnode->block;

        /* succ or pred */
        ptrvec_add(&giphase->arena, &block->preds, parent);
        ptrvec_add(&giphase->arena, &parent->succs, block);
    }

    giphase->curfunc->curblock = block;
}

void geninst_value(Phase *phase, AnalyFunction *analyfunc, Node *node)
{
    GenInstPhase *giphase = (GenInstPhase*)phase;
    Block *block = giphase->curfunc->curblock;
    Inst *inst = NULL;
    size_t regindex = giphase->curfunc->regindex;

    /* set reg on reg */
    table_setreg(&giphase->curfunc->valuetab,
                 &giphase->curfunc->funcarena,
                 node,
                 regindex);
    giphase->curfunc->regindex ++;

    switch (node->op) {
    case kNodeImm:
        inst = arena_malloc(&giphase->arena, sizeof(Inst));
        list_link_init(&inst->link);
        inst->code           = kInstLoadImm;
        inst->operand1.type  = kInstRegImm;
        inst->operand1.value = node->attr.imm;
        inst->operand2.type  = kInstRegNone;
        inst->dst.type       = kInstRegReg;
        inst->dst.value      = regindex;

        list_append(&block->insts, &inst->link);
        break;

    case kNodeCall: {
        size_t callreg;

        ptrvec_foreach(iter, &node->inputs) {
            Node *input = ptrvec_get(&node->inputs, iter);
            TableNode *tabnode = table_get(&giphase->curfunc->valuetab, input);

            assert(tabnode);

            /* get temp value */
            size_t tempindex = tabnode->reg;
            if (iter == 0) {
                callreg = tempindex;
                continue;
            }

            Inst *inst = arena_malloc(&giphase->arena, sizeof(Inst));
            list_link_init(&inst->link);
            inst->code          = kInstSetArg;
            inst->operand1.type = kInstRegImm;
            inst->operand1.value = iter;
            inst->operand2.type = kInstRegImm;
            inst->operand2.value = tempindex;
            inst->dst.type      = kInstRegNone;
            list_append(&block->insts, &inst->link);
        }
        inst = arena_malloc(&giphase->arena, sizeof(Inst));
        list_link_init(&inst->link);
        inst->code          = kInstCall;
        inst->operand1.type = kInstRegReg;
        inst->operand1.value = callreg;
        inst->operand2.type = kInstRegNone;
        inst->dst.type      = kInstRegReg;
        inst->dst.value     = regindex;
        list_append(&block->insts, &inst->link);
        break;
    }

    case kNodeLoad: {
        size_t firstreg, finalreg;

        ptrvec_foreach(iter, &node->inputs) {
            Node *input = ptrvec_get(&node->inputs, iter);
            TableNode *tabnode = table_get(&giphase->curfunc->valuetab, input);

            assert(tabnode);

            if (iter == 0) {
                finalreg = firstreg = tabnode->reg;
                continue;
            }

            /* 有第二个参数 */
            finalreg = giphase->curfunc->regindex ++;

            inst = arena_malloc(&giphase->arena, sizeof(Inst));
            list_link_init(&inst->link);
            inst->code           = kInstAdd;
            inst->operand1.type  = kInstRegReg;
            inst->operand1.value = firstreg;
            inst->operand2.type  = kInstRegReg;
            inst->operand2.value = tabnode->reg;
            inst->dst.type       = kInstRegReg;
            inst->dst.value      = finalreg;

            list_append(&block->insts, &inst->link);
        }

        inst = arena_malloc(&giphase->arena, sizeof(Inst));
        list_link_init(&inst->link);
        inst->code           = kInstLoadReg;
        inst->operand1.type  = kInstRegReg;
        inst->operand1.value = finalreg;
        inst->operand2.type  = kInstRegNone;
        inst->dst.type       = kInstRegReg;
        inst->dst.value      = regindex;

        list_append(&block->insts, &inst->link);
        break;
    }

    case kNodeLabel: {
        /* add label list */
        GenInstLabel *label = arena_malloc(&giphase->arena, sizeof(GenInstLabel));
        list_link_init(&label->link);
        label->name = arena_dup(&giphase->arena, node->attr.label.name,
                                strlen(node->attr.label.name) + 1);
        label->data = arena_dup(&giphase->arena, node->attr.label.data,
                                node->attr.label.datalen);
        label->datalen = node->attr.label.datalen;

        /* addinst */
        inst = arena_malloc(&giphase->arena, sizeof(Inst));
        list_link_init(&inst->link);
        inst->code          = kInstLoadLabel;
        inst->operand1.type = kInstRegLabel;
        inst->operand1.value = (size_t)label;
        inst->operand2.type = kInstRegNone;
        inst->dst.type      = kInstRegReg;
        inst->dst.value     = regindex;

        list_append(&block->insts, &inst->link);
        break;
    }
    case kNodeStore:
    case kNodeAdd:
    case kNodeShl:
    default:
        assert(!"bug!");
        break;
    }
}

GenInstPhase geninst = {
    {
        .visit_func = geninst_func,
        .visit_region = geninst_region,
        .visit_value  = geninst_value
    },
};

void geninst_init()
{
    /* init GenInstPhase */
    arena_init(&geninst.arena, 4096);
    list_init(&geninst.funclist);
    list_init(&geninst.labellist);
    geninst.curfunc = NULL;
}

void geninst_run(Analy *analy)
{
    phase_walk((Phase*)&geninst, analy);
}

void geninst_print()
{
    GenInstPhase *giphase = &geninst;

    list_foreach(listlink, giphase->funclist.first) {
        GenInstFunc *func = container_of(listlink, GenInstFunc, link);
        ptrvec_foreach(iter, &func->blockvec) {
            Block *block = ptrvec_get(&func->blockvec, iter);
            list_foreach(pos, block->insts.first) {
                Inst *inst = container_of(pos, Inst, link);
                inst_print(inst);
            }
        }
    }
}

static void instreg_print(InstReg *r)
{
    switch (r->type) {
    case kInstRegNone:
        break;
    case kInstRegReg:
        printf("r%ld", r->value);
        break;
    case kInstRegImm:
        printf("i%ld", r->value);
        break;
    case kInstRegLabel:
        printf("(%s)", ((GenInstLabel*)r->value)->name);
        break;
    }
}

void inst_print(Inst *inst)
{
    switch (inst->code) {
    case kInstLoadReg:
        printf("LoadReg");
        break;
    case kInstLoadImm:
        printf("LoadImm");
        break;
    case kInstLoadLabel:
        printf("LoadLabel");
        break;
    case kInstStore:
        printf("Store");
        break;
    case kInstJump:
        printf("Jump");
        break;
    case kInstCall:
        printf("Call");
        break;
    case kInstSetArg:
        printf("SetArg");
        break;
    case kInstAdd:
        printf("Add");
        break;
    default:
        printf("Unknown");
        break;
    }
    printf(" ");
    instreg_print(&inst->operand1);
    printf(" ");
    instreg_print(&inst->operand2);
    if (inst->dst.type != kInstRegNone) {
        printf(" -> ");
        instreg_print(&inst->dst);
    }
    printf("\n");
}