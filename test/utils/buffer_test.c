#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/buffer.h"

int main(int argc, char *argv[])
{
    Buffer buf;
    buffer_create1(&buf, "123123", 6);
    buffer_append(&buf, "567", 3);
    if (strcmp(buf.buffer, "123123567") != 0) {
        exit(-1);
    }
    buffer_free(&buf);
    return 0;
}