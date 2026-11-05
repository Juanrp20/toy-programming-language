#ifndef TOY_LL1_H
#define TOY_LL1_H

#include "set.h"
#include <stddef.h>

typedef struct {
    char **symbols;
    size_t count;
    size_t capacity;
} SymbolSeq;

typedef struct {
    SymbolSeq *items;
    size_t count;
    size_t capacity;
} AltList;

typedef struct {
    char *lhs;
    AltList alts;
    StringSet first;
    StringSet follow;
} Rule;

typedef struct {
    Rule *rules;
    size_t rule_count;
    size_t rule_capacity;
    StringSet non_terminals;
    StringSet nullable;
} LL1Grammar;

typedef struct {
    char *non_terminal;
    size_t alt1;
    size_t alt2;
    StringSet intersection;
} LL1Conflict;

typedef struct {
    LL1Conflict *items;
    size_t count;
    size_t capacity;
} LL1ConflictList;

void ll1_grammar_init(LL1Grammar *g);
int ll1_grammar_parse(LL1Grammar *g, const char *ebnf_text);
void ll1_grammar_calculate(LL1Grammar *g);
void ll1_grammar_find_conflicts(LL1Grammar *g, LL1ConflictList *confs);
void ll1_grammar_free(LL1Grammar *g);

void ll1_conflicts_init(LL1ConflictList *list);
void ll1_conflicts_free(LL1ConflictList *list);

#endif /* TOY_LL1_H */
