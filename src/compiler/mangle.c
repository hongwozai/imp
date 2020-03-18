#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include "mangle.h"

/* 判断是否是可以使用的字符 */
static bool isusechar(char c)
{
    return isalnum(c);
}

static void trans_char(char c, char buf[5])
{
    snprintf(buf, 5, "0x%X", c);
}

static size_t calc_len(const char *name)
{
    size_t len = 0;
    for (const char *p = name; *p != '\0'; p++) {
        if (!isusechar(*p)) {
            len += 4;
            continue;
        }
        len ++;
    }
    /* +1 \0 */
    return len + 1;
}

static void trans_string(const char *name, char *buf)
{
    size_t index = 0;
    for (const char *p = name; *p != '\0'; p++) {
        if (!isusechar(*p)) {
            trans_char(*p, buf + index);
            index += 4;
            continue;
        }
        buf[index++] = *p;
    }
    buf[index] = '\0';
}

char *mangle(Arena *arena, char *name)
{
    size_t len = calc_len(name);
    /* +1 因为前面加 _ */
    char *buf = arena_malloc(arena, len + 1);

    buf[0] = '_';
    trans_string(name, buf + 1);
    return buf;
}