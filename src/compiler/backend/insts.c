#include "insts.h"

Inst* inst_new(Arena *arena, const char *desc,
               InstReg *op1, InstReg *op2, InstReg *dst)
{
    Inst *inst = arena_malloc(arena, sizeof(Inst));
    list_link_init(&inst->link);
    inst->desc = arena_dup(arena, desc, strlen(desc) + 1);
    inst->reg[kInstOperand1] = *op1;
    inst->reg[kInstOperand2] = *op2;
    inst->reg[kInstDst] = *dst;
    return inst;
}

VirtualReg vreg_alloc(ModuleFunc *func)
{
    return func->vregindex ++;
}

InstReg *instreg_vset(InstReg *reg, VirtualReg vreg)
{
    reg->type = kInstRegVirtual;
    reg->vreg = vreg;
    reg->reg  = NULL;
    return reg;
}

InstReg *instreg_talloc(InstReg *reg, TargetReg *treg)
{
    reg->type = kInstRegTarget;
    reg->vreg = -1;
    reg->reg  = treg;
    return reg;
}

InstReg *instreg_unused(InstReg *reg)
{
    reg->type = kInstRegUnUsed;
    return reg;
}

InstReg *instreg_dummy(InstReg *reg)
{
    reg->type = kInstRegDummy;
    return reg;
}

bool instreg_isdummy(InstReg *reg)
{
    return reg->type == kInstRegDummy;
}

bool instreg_isvirtual(InstReg *reg)
{
    return reg->type == kInstRegVirtual;
}

bool instreg_isunused(InstReg *reg)
{
    return reg->type == kInstRegUnUsed;
}

bool instreg_istarget(InstReg *reg)
{
    return reg->type == kInstRegTarget;
}

/**
 * 固定用%1,%2,%r来代替寄存器
 * %1 operand1
 * %2 operand2
 * %r dst
 * movq $1, %r - > movq $1, v1
 */
void inst_dprint(FILE *out, Inst *inst)
{
    char buf[512];
    bool isswitch = false;
    size_t len = 0;

    buf[len] = '\0';
    for (char *p = inst->desc; *p != '\0'; p++) {
        if (isswitch) {
            int pos = -1;
            switch (*p) {
            case '1':
                pos = kInstOperand1;
                break;
            case '2':
                pos = kInstOperand2;
                break;
            case 'r':
                pos = kInstDst;
                break;
            default:
                buf[len++] = *p;
                isswitch = false;
                continue;
            }
            switch (inst->reg[pos].type) {
            case kInstRegVirtual:
                len += sprintf(buf + len, "v%d", inst->reg[pos].vreg);
                break;
            case kInstRegTarget:
                len += sprintf(buf + len, "%s", inst->reg[pos].reg->rep);
                break;
            case kInstRegUnUsed:
                len += sprintf(buf + len, "[unused]");
                break;
            case kInstRegDummy:
                len += sprintf(buf + len, "[dummy]");
                break;
            }
            isswitch = false;
            continue;
        }

        if (*p == '%') {
            isswitch = true;
            continue;
        }
        buf[len++] = *p;
    }
    buf[len++] = '\0';

    fprintf(out, "%s\n", buf);
}