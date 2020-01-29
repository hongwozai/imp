#include <stdio.h>
#include <stdlib.h>
#include "utils/list.h"

typedef struct persion {
    const char* name;
    int age;
} persion;

int container_of_test()
{
    persion p = {
        .name = "123123",
        .age = 11
    };

    if (p.name != container_of(&p.age, persion, age)->name) {
        exit(-1);
    }
    return 0;
}

struct node {
    const char *name;
    ListLink timelink;
    ListLink hashlink;
};

int list_test()
{
    struct node arr[100] = {
        [0] = {.name = "arr0", .timelink = INIT_LINK, .hashlink = INIT_LINK},
        [1] = {.name = "arr1", .timelink = INIT_LINK, .hashlink = INIT_LINK},
        [2] = {.name = "arr2", .timelink = INIT_LINK, .hashlink = INIT_LINK},
        [3] = {.name = "arr3", .timelink = INIT_LINK, .hashlink = INIT_LINK},
        [4] = {.name = "arr4", .timelink = INIT_LINK, .hashlink = INIT_LINK},
        [5] = {.name = "arr5", .timelink = INIT_LINK, .hashlink = INIT_LINK},
    };
    List list = INIT_LIST;
    list_push(&list, &arr[0].timelink);
    list_push(&list, &arr[1].timelink);
    list_push(&list, &arr[2].timelink);
    list_push(&list, &arr[3].timelink);
    list_push(&list, &arr[4].timelink);
    list_push(&list, &arr[5].timelink);

    list_foreach(temp, list.first) {
        struct node *n = container_of(temp, struct node, timelink);
        printf("n.name %s\n", n->name);
    }

    list_safe_foreach(temp, iter, list.first) {
        struct node *n = container_of(temp, struct node, timelink);
        printf("n.name %s\n", n->name);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    container_of_test();
    list_test();
    return 0;
}