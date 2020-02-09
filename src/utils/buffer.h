#ifndef SRC_UTILS_BUFFER_H
#define SRC_UTILS_BUFFER_H

#include <stddef.h>

typedef struct Buffer {
    char *buffer;
    size_t size;
    size_t capacity;
} Buffer;

void buffer_create(Buffer *buf, size_t size);
void buffer_create1(Buffer *buf, const char *str, size_t size);
void buffer_append(Buffer *buf, const char *str, size_t size);
void buffer_appendchar(Buffer *buf, char ch);
void buffer_appendbuf(Buffer *buf, Buffer *other);
void buffer_reserved(Buffer *buf, size_t size);
void buffer_free(Buffer *buf);

static inline void buffer_clear(Buffer *buf) { buf->size = 0; }

#endif /* SRC_UTILS_BUFFER_H */