#include "insts.h"

Inst* inst_newv(Arena *arena, const char *desc,
                VirtualReg operand1, VirtualReg operand2, VirtualReg dst)
{
    Inst *inst = arena_malloc(arena, sizeof(Inst));
    list_link_init(&inst->link);
    inst->desc = arena_dup(arena, desc, strlen(desc) + 1);
    inst->isvirtual = true;
    inst->u.vreg[kInstOperand1] = operand1;
    inst->u.vreg[kInstOperand2] = operand2;
    inst->u.vreg[kInstDst] = dst;
    return inst;
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
    for (char *p = inst->desc; *p != NULL; p++) {
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
            len += sprintf(buf + len, "v%d", inst->u.vreg[pos]);
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