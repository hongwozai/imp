#include "global.h"

/**
 * main函数入口
 */
extern intptr_t _main();

int main(int argc, char *argv[])
{
    intptr_t ret = 0;
    global_init();
    ret = _main();
    global_destroy();
    return ret;
}