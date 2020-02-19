#include <assert.h>
#include <stdio.h>
#include "nodes.h"

Node* node_new(Arena *arena, Opcode op)
{
    Node *node = arena_malloc(arena, sizeof(Node));
    node->op = op;
    node->mode = kModeTop;
    ptrvec_init(arena, &node->inputs, 0);
    ptrvec_init(arena, &node->ctrls, 2);
    list_init(&node->uses);
    list_init(&node->ctrluses);
    node->attr.imm = 0;
    node->next = NULL;
    return node;
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
    default:
        break;
    }
    fprintf(out, ")");
}
