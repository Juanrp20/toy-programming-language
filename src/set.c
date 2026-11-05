#include "set.h"
#include <stdlib.h>
#include <string.h>

void set_init(StringSet *s) {
    s->items = NULL;
    s->count = 0;
    s->capacity = 0;
}

int set_contains(const StringSet *s, const char *str) {
    if (!s || s->count == 0 || !str) return 0;
    for (size_t i = 0; i < s->count; i++) {
        if (strcmp(s->items[i], str) == 0) {
            return 1;
        }
    }
    return 0;
}

int set_add(StringSet *s, const char *str) {
    if (!str || set_contains(s, str)) {
        return 0;
    }
    if (s->count >= s->capacity) {
        size_t new_cap = s->capacity == 0 ? 8 : s->capacity * 2;
        char **new_items = (char **)realloc(s->items, new_cap * sizeof(char *));
        if (!new_items) return 0;
        s->items = new_items;
        s->capacity = new_cap;
    }
    /* Inserción ordenada */
    size_t pos = s->count;
    while (pos > 0 && strcmp(s->items[pos - 1], str) > 0) {
        s->items[pos] = s->items[pos - 1];
        pos--;
    }
    size_t len = strlen(str);
    s->items[pos] = (char *)malloc(len + 1);
    memcpy(s->items[pos], str, len + 1);
    s->count++;
    return 1;
}

int set_union(StringSet *dest, const StringSet *src) {
    if (!src) return 0;
    int changed = 0;
    for (size_t i = 0; i < src->count; i++) {
        if (set_add(dest, src->items[i])) {
            changed = 1;
        }
    }
    return changed;
}

void set_intersect(StringSet *dest, const StringSet *s1, const StringSet *s2) {
    set_init(dest);
    if (!s1 || !s2) return;
    for (size_t i = 0; i < s1->count; i++) {
        if (set_contains(s2, s1->items[i])) {
            set_add(dest, s1->items[i]);
        }
    }
}

int set_is_subset(const StringSet *sub, const StringSet *parent) {
    if (!sub) return 1;
    for (size_t i = 0; i < sub->count; i++) {
        if (!set_contains(parent, sub->items[i])) {
            return 0;
        }
    }
    return 1;
}

void set_clear(StringSet *s) {
    if (!s) return;
    for (size_t i = 0; i < s->count; i++) {
        if (s->items[i]) free(s->items[i]);
    }
    s->count = 0;
}

void set_free(StringSet *s) {
    if (!s) return;
    set_clear(s);
    if (s->items) {
        free(s->items);
        s->items = NULL;
    }
    s->capacity = 0;
}
