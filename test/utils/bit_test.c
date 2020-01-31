#include <stdio.h>
#include <stdlib.h>
#include "utils/bit.h"

int main(int argc, char *argv[])
{
    uint8_t set[10] = {0};

    bit_set(set, 10);
    if (0 != bit_check(set, 8)) {
        exit(-1);
    }
    if (!bit_check(set, 10)) {
        exit(-1);
    }
    bit_clear(set, 10);
    if (bit_check(set, 10)) {
        exit(-1);
    }
    return 0;
}