#ifndef SRC_COMPILER_PHASE_H
#define SRC_COMPILER_PHASE_H

#include "analysis.h"

typedef struct Phase Phase;

struct Phase {
    void (*visit_region)(Phase *phase, AnalyFunction *analyfunc, Node *node);
    void (*visit_value)(Phase *phase, AnalyFunction *analyfunc, Node *node);
};

void phase_run(Analy *analy);

#endif /* SRC_COMPILER_PHASE_H */