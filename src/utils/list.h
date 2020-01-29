#ifndef SRC_UTILS_LIST_H
#define SRC_UTILS_LIST_H

#include <stddef.h>

#ifdef USE_TYPEOF
#define container_of(ptr, type, member) ({                      \
            const typeof (((type*)0)->member) *__mptr = (ptr);  \
            (type*)((char*)__mptr - offsetof(type, member));    \
        })

#else
#define container_of(ptr, type, member) ({                  \
            (type*)((char*)(ptr) - offsetof(type, member)); \
        })
#endif

typedef struct ListLink {
    struct ListLink *prev;
    struct ListLink *next;
} ListLink;

typedef struct List {
    ListLink *first;
    ListLink *last;
} List;

#define INIT_LIST { NULL, NULL }
#define INIT_LINK { NULL, NULL }

static inline void list_init(List *list)
{
    list->first = NULL;
    list->last  = NULL;
}

static inline void list_link_init(ListLink *link)
{
    link->prev = NULL;
    link->next = NULL;
}

/**
 * 添加到pos之后
 * pos为空，则添加到链表头部
 * pos为list->last则添加到链表尾部
 */
static inline void list_add(List *list, ListLink *pos, ListLink *link)
{
    if (!link) return;

    link->prev = pos;
    link->next = (pos == NULL) ? list->first : pos->next;

    if (link->prev == NULL) list->first = link;
    else                    link->prev->next = link;
    if (link->next == NULL) list->last = link;
    else                    link->next->prev = link;
}

static inline void list_del(List *list, ListLink *link)
{
    if (!link) return;

    if (link->prev == NULL) list->first = link->next;
    else                    link->prev->next = link->next;
    if (link->next == NULL) list->last = link->prev;
    else                    link->next->prev = link->prev;
}

static inline void list_push(List *list, ListLink *link)
{
    list_add(list, NULL, link);
}

static inline void list_append(List *list, ListLink *link)
{
    list_add(list, list->last, link);
}

static inline ListLink* list_pop(List *list)
{
    ListLink *link = list->first;
    list_del(list, list->first);
    return link;
}

/**
 * @param pos 当前正在遍历的节点
 * @param head 第一个开始遍历的节点
 */
#define list_foreach(pos, head)                                 \
    for (ListLink *pos = (head); pos != NULL; pos = pos->next)

#define list_safe_foreach(pos, iter, head)              \
    for (ListLink *pos = (head), *iter = pos->next;     \
         pos != NULL;                                   \
         pos = iter, iter = (pos ? pos->next : NULL))

#endif /* SRC_UTILS_LIST_H */