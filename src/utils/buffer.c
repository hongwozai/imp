#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "buffer.h"

void buffer_create(Buffer *buf, size_t size)
{
    buf->buffer = (char*)malloc(size + 1);
    if (!buf->buffer) {
        return;
    }
    buf->size = size;
    buf->capacity = size + 1;
}

void buffer_create1(Buffer *buf, const char *str, size_t size)
{
    buffer_create(buf, size);
    if (!buf->buffer) {
        return;
    }
    memcpy(buf->buffer, str, size);
    buf->buffer[buf->size] = 0;
}

void buffer_append(Buffer *buf, const char *str, size_t size)
{
    /* reserved space */
    buffer_reserved(buf, buf->size + size);
    /* copy */
    memcpy(buf->buffer + buf->size, str, size);
    buf->size += size;
    buf->buffer[buf->size] = 0;
}

void buffer_appendbuf(Buffer *buf, Buffer *other)
{
    buffer_append(buf, other->buffer, other->size);
}

void buffer_reserved(Buffer *buf, size_t size)
{
    if (buf->capacity <= size) {
        buf->buffer = realloc(buf->buffer, size + 1);
        buf->capacity = size + 1;
    }
}

void buffer_free(Buffer *buf)
{
    free(buf->buffer);
    buf->buffer = NULL;
    buf->size = 0;
    buf->capacity = 0;
}
