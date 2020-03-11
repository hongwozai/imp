#ifndef IMP_SRC_COMPILER_PHASE_H
#define IMP_SRC_COMPILER_PHASE_H

#include "analysis.h"

typedef struct Phase Phase;

struct Phase {
    void (*visit_func)(Phase *phase, AnalyFunction *analyfunc);
    void (*visit_region)(Phase *phase, AnalyFunction *analyfunc, Node *node);
    void (*visit_value)(Phase *phase, AnalyFunction *analyfunc, Node *node);
};

void phase_walk(Phase *phase, Analy *analy);
void phase_run(Analy *analy);

#endif /* IMP_SRC_COMPILER_PHASE_H */
