#include "arena.h"
#include <stdlib.h>
#include <string.h>

void arena_init(Arena *a) {
    a->count = 0;
    a->capacity = 64;
    a->ptrs = (void **)malloc(a->capacity * sizeof(void *));
}

void *arena_alloc(Arena *a, size_t size) {
    void *ptr = malloc(size);
    if (!ptr) return NULL;

    if (a->count >= a->capacity) {
        a->capacity *= 2;
        a->ptrs = (void **)realloc(a->ptrs, a->capacity * sizeof(void *));
    }
    a->ptrs[a->count++] = ptr;
    return ptr;
}

char *arena_strdup(Arena *a, const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *copy = (char *)arena_alloc(a, len + 1);
    if (copy) {
        memcpy(copy, str, len + 1);
    }
    return copy;
}

void arena_free(Arena *a) {
    if (!a) return;
    for (size_t i = 0; i < a->count; i++) {
        if (a->ptrs[i]) {
            free(a->ptrs[i]);
        }
    }
    if (a->ptrs) {
        free(a->ptrs);
        a->ptrs = NULL;
    }
    a->count = 0;
    a->capacity = 0;
}
