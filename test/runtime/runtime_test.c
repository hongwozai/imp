#include <stdio.h>
#include <stdlib.h>
#include "runtime/global.h"
#include "runtime/reader.h"

int main(int argc, char *argv[])
{
    Reader reader;
    global_init();
    if (!reader_open(&reader, argv[1])) {
        perror("error: ");
        exit(-1);
    }
    printf("%s\n", argv[1]);
    Object *obj = reader_read(&reader);
    if (gettype(obj) != kCons) {
        exit(-1);
    }
    print_object(stdout, obj);
    reader_close(&reader);
    global_destroy();
    return 0;
}