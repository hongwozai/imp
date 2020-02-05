#include <stdio.h>
#include "phase.h"
#include "analysis.h"
#include "runtime/global.h"
#include "runtime/reader.h"

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

    phase_run(&analy);

    analysis_destroy(&analy);
    reader_close(&reader);
    global_destroy();
    return 0;
}