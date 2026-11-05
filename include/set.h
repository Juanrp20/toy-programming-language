#ifndef TOY_SET_H
#define TOY_SET_H

#include <stddef.h>

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} StringSet;

void set_init(StringSet *s);
int set_add(StringSet *s, const char *str);
int set_contains(const StringSet *s, const char *str);
int set_union(StringSet *dest, const StringSet *src);
void set_intersect(StringSet *dest, const StringSet *s1, const StringSet *s2);
int set_is_subset(const StringSet *sub, const StringSet *parent);
void set_clear(StringSet *s);
void set_free(StringSet *s);

#endif /* TOY_SET_H */
