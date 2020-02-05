#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "gc.h"
#include "reader.h"
#include "global.h"
#include "object.h"
#include "panic.h"
#include "utils/buffer.h"

static Object* read_list(Reader *reader);
static Object* read_string(Reader *reader);
static Object* read_symbol(Reader *reader);
static Object* read_number(Reader *reader, int sign);

/**
 * @return 返回是否为分隔符
 */
static int issep(int ch)
{
    switch (ch) {
    case '(': case ')':
    case ' ': case '\t':
    case '\v': case '\n': case '\r': case '\f':
        return 1;
    }
    return 0;
}

/**
 * @return >= 0 返回转义的字符
 *         < 0  代表该字符直接忽略
 */
static int trans(int ch)
{
    switch (ch) {
    case 'n': return '\n';
    case 't': return '\t';
    case 'v': return '\v';
    case 'a': return '\a';
    case 'r': return '\r';
    case '\n': return -1;
    case 'b': return '\b';
    case '0': return '\0';
    case 'f': return '\f';
    default: return ch;
    }
}

static inline int readchar(Reader *reader)
{
    return (reader->la = fgetc(reader->in));
}

static inline void backchar(Reader *reader)
{
    if (reader->la != -1) {
        ungetc(reader->la, reader->in);
    }
}

bool reader_open(Reader *reader, const char *file)
{
    reader->in = NULL;
    reader->la = -1;

    reader->in = fopen(file, "rb");
    if (!reader->in) {
        return false;
    }
    return true;
}

void reader_close(Reader *reader)
{
    if (reader->in) {
        fflush(reader->in);
        fclose(reader->in);
    }
    reader->in = NULL;
    reader->la = -1;
}

Object *reader_read(Reader *reader)
{
    int ch;

    for (;;) {
        ch = readchar(reader);

        /* comment */
        if (ch == ';') {
            while (ch != '\n' && ch != -1) {
                ch = readchar(reader);
            }
        }

        /* eof */
        if (ch == -1) {
            return gc_new(&impgc, kEof, sizeof(ValueObject));
        }

        /* skip space */
        if (isspace(ch)) {
            continue;
        }

        /* unsigned number */
        if (isdigit(ch)) {
            backchar(reader);
            return read_number(reader, 0);
        }

        /* signed number */
        if (ch == '+' || ch == '-') {
            int second_char = readchar(reader);
            backchar(reader);
            if (isdigit(second_char)) {
                return read_number(reader, ch);
            }
            /* 第二个字节的缓存 */
            reader->la = ch;
            backchar(reader);
            return read_symbol(reader);
        }

        /* string */
        if (ch == '"') {
            return read_string(reader);
        }

        /* list */
        if (ch == '(') {
            return read_list(reader);
        }

        backchar(reader);
        return read_symbol(reader);
    }
}

static Object* read_number(Reader *reader, int sign)
{
    int ch;
    int isfloat = 0;
    Buffer buf;

    buffer_create(&buf, 64);

    /* 加入符号 */
    if (sign != 0) {
        buffer_appendchar(&buf, sign);
    }

    for (;;) {
        ch = readchar(reader);

        /* 遇到EOF或者（不是字符且不是浮点数的.） */
        if (ch < 0 || (!isdigit(ch) && !(isfloat == 0 && ch == '.'))) {
            if (buf.size == 0) {
                /* NOTE: 这里没有消费字符，如果有bug会导致无限循环 */
                panic("bug! have bug!");
            }

            buffer_appendchar(&buf, 0);

            /* 生成数字对象 */
            Object *obj;

            if (isfloat) {
                obj = gc_new(&impgc, kFixFloat, sizeof(ValueObject));
                getvalue(obj).fixfloat = strtod(buf.buffer, NULL);
            } else {
                obj = gc_new(&impgc, kFixInt, sizeof(ValueObject));
                getvalue(obj).fixint = strtol(buf.buffer, NULL, 10);
            }

            /* 如果有溢出 */
            if (errno == ERANGE) {
                panic("ERANGE! overflow.");
            }

            /* 读了其他字符(非EOF)，进行退回 */
            if (ch >= 0) {
                backchar(reader);
            }
            buffer_free(&buf);
            return obj;
        }

        /* 是数字，或者是符号，或者是.(并且当前还没有其他的.) */
        if (isdigit(ch) || (ch == '.' && isfloat == 0)) {
            buffer_appendchar(&buf, ch);
            /* 设置为浮点数 */
            if (ch == '.') {
                isfloat = 1;
            }
        } else {
            /* 数字中有非数字 */
            panic("syntax error(not number char)");
        }
    }

    panic("not fall througth");
}

static Object* read_list(Reader *reader)
{
    int ch;
    Object *ret = imp_nil, *last = imp_nil;

    for (;;) {
        ch = readchar(reader);

        if (isspace(ch)) {
            continue;
        }

        if (ch < 0) {
            /* 列表没有最后的右括号 */
            panic("syntax error! havn't )");
        }

        if (ch == ')') {
            /* 列表结束 */
            return ret;
        }

        backchar(reader);
        Object *elem = reader_read(reader);

        /* 将新的cons连接到last上 */
        ConsObject *new =
            (ConsObject*)gc_new(&impgc, kCons, sizeof(ConsObject));
        new->car = elem;
        new->cdr = imp_nil;

        if (last == imp_nil) {
            ret = last = (Object*)new;
        } else {
            ConsObject *cons = (ConsObject*)last;
            cons->cdr = (Object*)new;
            last = cons->cdr;
        }
    }
    return ret;
}

static Object* read_string(Reader *reader)
{
    Buffer buf;

    /* 从没转义状态开始 */
    char slash_state = 0;

    buffer_create(&buf, 64);
    for (;;) {
        int ch = readchar(reader);

        if (ch < 0) {
            /* 字符串没有完结的最后的双引号 */
            panic("syntax error! string not finished!");
        }

        /* 转义状态 */
        if (slash_state) {
            slash_state = 0;

            /* 此处改变ch */
            ch = trans(ch);
            if (ch < 0) {
                /* <0时，忽略当前字符 */
                continue;
            }
            buffer_appendchar(&buf, ch);
            continue;
        }

        /* 非转义状态 */
        if (ch == '\\') {
            /* 进入转义状态 */
            slash_state = 1;
            continue;
        }

        /* 结束 */
        if (ch == '"') {
            /* 生成字符串 */
            return gc_create_string(&impgc, buf.buffer, buf.size);
        }

        buffer_appendchar(&buf, ch);
    }
}

static Object* read_symbol(Reader *reader)
{
    int ch;
    Buffer buf;
    Object *obj = imp_nil;

    buffer_create(&buf, 64);
    for (;;) {
        ch = readchar(reader);

        /* 遇到分隔符，生成符号并返回 */
        if (ch < 0 || issep(ch)) {
            if (strncmp(buf.buffer, "true", 4) == 0) {
                obj = imp_true;
            } else if (strncmp(buf.buffer, "false", 5) == 0) {
                obj = imp_false;
            } else if (strncmp(buf.buffer, "nil", 3) == 0) {
                obj = imp_nil;
            } else {
                /* symbol */
                obj = gc_create_symbol(&impgc, buf.buffer, buf.size, false);
            }

            if (ch >= 0) {
                backchar(reader);
            }
            buffer_free(&buf);
            return obj;
        }

        buffer_appendchar(&buf, ch);
    }

    panic("not fall througth");
}
