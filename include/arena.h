#ifndef TOY_ARENA_H
#define TOY_ARENA_H

#include <stddef.h>

typedef struct {
    void **ptrs;
    size_t count;
    size_t capacity;
} Arena;

void arena_init(Arena *a);
void *arena_alloc(Arena *a, size_t size);
char *arena_strdup(Arena *a, const char *str);
void arena_free(Arena *a);

#endif /* TOY_ARENA_H */
