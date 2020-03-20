#ifndef IMP_SRC_COMPILER_PHASE_PHASE_H
#define IMP_SRC_COMPILER_PHASE_PHASE_H

#include "analysis.h"

typedef struct Phase Phase;
struct Phase {
    void (*visit_func)  (Phase *phase, AnalyFunction *func);
    void (*visit_region)(Phase *phase, AnalyFunction *func, Node *node);
    void (*visit_value) (Phase *phase, AnalyFunction *func, Node *node);

    /* 遍历中当前的模式 */
    WalkMode mode;
};

WalkMode phase_nextmode(Phase *phase);
void phase_run(Phase *phase, Analy *analy);

#endif /* IMP_SRC_COMPILER_PHASE_PHASE_H */