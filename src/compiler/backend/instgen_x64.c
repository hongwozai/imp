#include <assert.h>
#include "module.h"
#include "instgen.h"
#include "compiler/nodes.h"

static VirtualReg allocvreg(ModuleFunc *func)
{
    return func->vregindex++;
}

static VirtualReg allocnonvreg()
{
    return -1;
}

static bool isvreg(VirtualReg r)
{
    return r >= 0;
}

static Inst *newinst2v(Arena *arena, const char *desc,
                       VirtualReg src, VirtualReg dst)
{
    Inst *inst = arena_malloc(arena, sizeof(Inst));
    list_link_init(&inst->link);
    inst->desc = arena_dup(arena, desc, strlen(desc) + 1);
    inst->isvirtual = true;
    inst->u.vreg[kInstOperand1] = src;
    inst->u.vreg[kInstOperand2] = allocnonvreg();
    inst->u.vreg[kInstDst] = dst;
    return inst;
}

/**
 * reg: (Load (Add vreg imm)) -> mov $imm(vreg), vreg
 * reg: (Call (Label) ...) -> call (label); mov $rax, vreg
 * reg: (Call reg ...) -> call reg; mov $rax, vreg
 */
void instgen_x64(ModuleFunc *func, Block *block, Node *node)
{
    switch (node->op) {
    case kNodeLoad:
    case kNodeCall:
    case kNodeAdd:
    case kNodeLabel: {
        VirtualReg vreg = allocvreg(func);
        Inst *inst = newinst2v(&func->arena, "mov (%s), %{dst}",
                               node->attr.label.name, vreg);
        list_append(&block->insts, &inst->link);
        break;
    }
    default:
        assert(!"bug!");
        break;
    }
}