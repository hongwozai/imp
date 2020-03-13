#include <assert.h>
#include <stdio.h>
#include "nodes.h"

Node* node_new(Arena *arena, Opcode op)
{
    Node *node = arena_malloc(arena, sizeof(Node));
    node->op = op;
    ptrvec_init(arena, &node->inputs, 0);
    ptrvec_init(arena, &node->ctrls, 2);
    list_init(&node->uses);
    list_init(&node->ctrluses);
    node->attr.imm = 0;
    node->mode = kModeTop;
    node->next = NULL;
    return node;
}

Node* node_newlabel(Arena *arena, char *name, char *data, size_t len)
{
    Node *label = node_new(arena, kNodeLabel);
    label->attr.label.name = arena_dup(arena, name, strlen(name) + 1);
    label->attr.label.data = arena_dup(arena, data, len);
    label->attr.label.datalen = len;
    return label;
}

void node_use(Arena *arena, Node *self, Node *usenode, size_t index, bool isctrl)
{
    Use *use = arena_malloc(arena, sizeof(Use));
    list_link_init(&use->link);
    use->index = index;
    use->node = usenode;

    list_append(isctrl ? &self->ctrluses : &self->uses, &use->link);
}

void node_unuse(Node *self, Node *other, size_t index, bool isctrl)
{
    List *uses = isctrl ? &self->ctrluses : &self->uses;
    list_safe_foreach(pos, iter, uses->first) {
        Use *use = container_of(pos, Use, link);
        if (use->node == other && use->index == index) {
            list_del(uses, pos);
        }
    }
}

void node_addinput(Arena *arena, Node *self, Node *other, bool isctrl)
{
    PtrVec *inputs = isctrl ? &self->ctrls : &self->inputs;
    size_t index = ptrvec_add(arena, inputs, other);

    node_use(arena, other, self, index, isctrl);
}

void node_replaceinput(Arena *arena, Node *self, bool isctrl,
                       size_t index, Node *other)
{
    PtrVec *inputs = isctrl ? &self->ctrls : &self->inputs;
    assert(index < ptrvec_count(inputs));

    node_unuse(ptrvec_get(inputs, index), self, index, isctrl);
    ptrvec_set(inputs, index, other);
    node_use(arena, other, self, index, isctrl);
}

/* 替代节点，替代使用层，替代控制层 */
void node_vreplace(Arena *arena, Node *self, Node *other)
{
    Node *top = other;

    /* 找到other最高的那个控制节点（也可能是自己），只跟随第一个控制节点 */
    while (ptrvec_count(&top->ctrls) != 0) {
        top = ptrvec_get(&top->ctrls, 0);
    }

    /* 将self的控制input转到other上 */
    ptrvec_foreach(pos, &self->ctrls) {
        Node *ctrlinput = ptrvec_get(&self->ctrls, pos);
        node_addinput(arena, top, ctrlinput, true);
    }

    /* 将所有的ctrluses转到other上 */
    list_foreach(pos, self->ctrluses.first) {
        Use *use = container_of(pos, Use, link);
        node_replaceinput(arena, use->node, true, use->index, other);
    }

    /* 将所有的uses转到other */
    list_foreach(pos, self->uses.first) {
        Use *use = container_of(pos, Use, link);
        node_replaceinput(arena, use->node, false, use->index, other);
    }
}

void node_dprint(FILE *out, Node *self)
{
    fprintf(out, "(");
    switch (self->op) {
    case kNodeCallObj:
        fprintf(out, "CallObj");
        break;
    case kNodeConstObj:
        fprintf(out, "ConstObj");
        break;
    case kNodeGlobalObj:
        fprintf(out, "GlobalObj");
        break;
    case kNodePhi:
        fprintf(out, "Phi");
        break;
    case kNodeRegion:
        fprintf(out, "Region");
        break;
    case kNodeCall:
        fprintf(out, "Call");
        break;
    case kNodeAdd:
        fprintf(out, "Add");
        break;
    case kNodeLoad:
        fprintf(out, "Load");
        break;
    case kNodeStore:
        fprintf(out, "Store");
        break;
    case kNodeImm:
        fprintf(out, "Imm");
        break;
    case kNodeLabel:
        fprintf(out, "Label");
        break;
    default:
        fprintf(out, "op%d", self->op);
    }
    if (!ptrvec_empty(&self->inputs)) {
        fprintf(out, " ");
    }

    /* inputs */
    ptrvec_foreach(iter, &self->inputs) {
        node_dprint(out, ptrvec_get(&self->inputs, iter));
        if (iter != ptrvec_count(&self->inputs) - 1) {
            fprintf(out, " ");
        }
    }
    /* value */
    switch (self->op) {
    case kNodeConstObj:
    case kNodeGlobalObj:
        fprintf(out, " ");
        print_object(out, self->attr.obj);
        break;
    case kNodeImm:
        fprintf(out, " %ld", self->attr.imm);
        break;
    case kNodeLabel:
        fprintf(out, " %s", self->attr.label.name);
        break;
    default:
        break;
    }
    fprintf(out, ")");
}
