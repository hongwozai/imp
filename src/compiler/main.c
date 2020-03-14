#include <stdio.h>
#include "analysis.h"
#include "phase/phase.h"
#include "runtime/global.h"
#include "runtime/reader.h"

#include "phase/print_phase.h"
#include "phase/deobj_phase.h"

#include "backend/module.h"
#include "backend/instgen.h"

int main(int argc, char *argv[])
{
    Reader reader;
    global_init();
    if (!reader_open(&reader, argv[1])) {
        return -1;
    }

    Object *obj;
    Analy analy;
    analysis_init(&analy);
    while (true) {
        obj = reader_read(&reader);
        print_object(stdout, obj);
        if (gettype(obj) == kEof) {
            break;
        }
        analysis_analy(&analy, obj);
    }

    printf("\n");
    phase_run(deobj_phase, &analy);
    /* phase_run(print_phase, &analy); */

    Module module;
    instgen_run(&analy, &module);
    /* geninst_init(); */
    /* geninst_run(&analy); */
    /* geninst_print(); */

    analysis_destroy(&analy);
    reader_close(&reader);
    global_destroy();
    return 0;
}