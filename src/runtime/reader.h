#ifndef IMP_SRC_RUNTIME_READER_H
#define IMP_SRC_RUNTIME_READER_H

#include <stdio.h>
#include <stdbool.h>

#include "object.h"

typedef struct Reader {
    FILE *in;
    /* lookahead */
    int la;
} Reader;

bool reader_open(Reader *reader, const char *file);
void reader_close(Reader *reader);
Object *reader_read(Reader *reader);

#endif /* IMP_SRC_RUNTIME_READER_H */