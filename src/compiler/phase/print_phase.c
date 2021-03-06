#include "print_phase.h"

static void print_value(Phase *phase, AnalyFunction *func, Node *node)
{
    node_dprint(stdout, node);
    fprintf(stdout, "\n");
}

static void print_region(Phase *phase, AnalyFunction *func, Node *node)
{
    node_dprint(stdout, node);
    fprintf(stdout, "\n");
}


static Phase phase = {
    .visit_func = NULL,
    .visit_region = print_region,
    .visit_value = print_value,
};

Phase *print_phase = &phase;
