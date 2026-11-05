#include "ll1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *my_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *p = (char *)malloc(len + 1);
    if (p) memcpy(p, s, len + 1);
    return p;
}

typedef enum {
    TOK_EBNF_EOF,
    TOK_EBNF_ARROW,
    TOK_EBNF_LBRACE,
    TOK_EBNF_RBRACE,
    TOK_EBNF_LBRACKET,
    TOK_EBNF_RBRACKET,
    TOK_EBNF_LPAR,
    TOK_EBNF_RPAR,
    TOK_EBNF_PIPE,
    TOK_EBNF_NT,
    TOK_EBNF_T,
    TOK_EBNF_ID
} EbnfTokKind;

typedef struct {
    EbnfTokKind kind;
    char *val;
} EbnfToken;

typedef struct {
    EbnfToken *items;
    size_t count;
    size_t capacity;
} EbnfTokenList;

static void tok_list_init(EbnfTokenList *l) {
    l->items = NULL;
    l->count = 0;
    l->capacity = 0;
}

static void tok_list_push(EbnfTokenList *l, EbnfTokKind kind, const char *val) {
    if (l->count >= l->capacity) {
        size_t new_cap = l->capacity == 0 ? 32 : l->capacity * 2;
        l->items = (EbnfToken *)realloc(l->items, new_cap * sizeof(EbnfToken));
        l->capacity = new_cap;
    }
    l->items[l->count].kind = kind;
    l->items[l->count].val = my_strdup(val);
    l->count++;
}

static void tok_list_free(EbnfTokenList *l) {
    if (!l) return;
    for (size_t i = 0; i < l->count; i++) {
        if (l->items[i].val) free(l->items[i].val);
    }
    if (l->items) free(l->items);
    l->items = NULL;
    l->count = 0;
    l->capacity = 0;
}

static void seq_init(SymbolSeq *s) {
    s->symbols = NULL;
    s->count = 0;
    s->capacity = 0;
}

static void seq_push(SymbolSeq *s, const char *sym) {
    if (s->count >= s->capacity) {
        size_t new_cap = s->capacity == 0 ? 4 : s->capacity * 2;
        s->symbols = (char **)realloc(s->symbols, new_cap * sizeof(char *));
        s->capacity = new_cap;
    }
    s->symbols[s->count++] = my_strdup(sym);
}

static SymbolSeq seq_clone(const SymbolSeq *s) {
    SymbolSeq copy;
    seq_init(&copy);
    for (size_t i = 0; i < s->count; i++) {
        seq_push(&copy, s->symbols[i]);
    }
    return copy;
}

static void seq_free(SymbolSeq *s) {
    if (!s) return;
    for (size_t i = 0; i < s->count; i++) {
        if (s->symbols[i]) free(s->symbols[i]);
    }
    if (s->symbols) free(s->symbols);
    s->symbols = NULL;
    s->count = 0;
    s->capacity = 0;
}

static void alts_init(AltList *a) {
    a->items = NULL;
    a->count = 0;
    a->capacity = 0;
}

static void alts_push(AltList *a, SymbolSeq seq) {
    if (a->count >= a->capacity) {
        size_t new_cap = a->capacity == 0 ? 4 : a->capacity * 2;
        a->items = (SymbolSeq *)realloc(a->items, new_cap * sizeof(SymbolSeq));
        a->capacity = new_cap;
    }
    a->items[a->count++] = seq;
}

static void alts_free(AltList *a) {
    if (!a) return;
    for (size_t i = 0; i < a->count; i++) {
        seq_free(&a->items[i]);
    }
    if (a->items) free(a->items);
    a->items = NULL;
    a->count = 0;
    a->capacity = 0;
}

/* Helper para contadores de auxiliares */
typedef struct {
    char *base;
    int counter;
} AuxCounter;

typedef struct {
    AuxCounter *items;
    size_t count;
    size_t capacity;
} AuxMap;

static char *make_aux_name(AuxMap *map, const char *orig_lhs) {
    /* base = orig_lhs.strip("<>").split("_aux")[0] */
    char stripped[256];
    size_t len = strlen(orig_lhs);
    size_t s = 0, e = len;
    if (len > 0 && orig_lhs[0] == '<') s = 1;
    if (len > 1 && orig_lhs[len - 1] == '>') e = len - 1;

    size_t k = 0;
    for (size_t i = s; i < e; i++) {
        if (orig_lhs[i] == '_' && strncmp(&orig_lhs[i], "_aux", 4) == 0) {
            break;
        }
        stripped[k++] = orig_lhs[i];
    }
    stripped[k] = '\0';

    int count = 1;
    int found = 0;
    for (size_t i = 0; i < map->count; i++) {
        if (strcmp(map->items[i].base, stripped) == 0) {
            map->items[i].counter++;
            count = map->items[i].counter;
            found = 1;
            break;
        }
    }
    if (!found) {
        if (map->count >= map->capacity) {
            size_t new_cap = map->capacity == 0 ? 8 : map->capacity * 2;
            map->items = (AuxCounter *)realloc(map->items, new_cap * sizeof(AuxCounter));
            map->capacity = new_cap;
        }
        map->items[map->count].base = my_strdup(stripped);
        map->items[map->count].counter = 1;
        map->count++;
    }

    char res[512];
    snprintf(res, sizeof(res), "<%s_aux%d>", stripped, count);
    return my_strdup(res);
}

static void aux_map_free(AuxMap *map) {
    if (!map) return;
    for (size_t i = 0; i < map->count; i++) {
        free(map->items[i].base);
    }
    if (map->items) free(map->items);
    map->items = NULL;
    map->count = 0;
    map->capacity = 0;
}

static void tokenize_ebnf(const char *text, EbnfTokenList *tokens) {
    tok_list_init(tokens);
    const char *p = text;

    while (*p) {
        /* Line-by-line processing for comments */
        const char *line_start = p;
        while (*p && *p != '\n') p++;
        size_t line_len = p - line_start;
        if (*p == '\n') p++;

        char line[2048];
        if (line_len >= sizeof(line)) line_len = sizeof(line) - 1;
        memcpy(line, line_start, line_len);
        line[line_len] = '\0';

        char *comment = strstr(line, "--");
        if (comment) {
            *comment = '\0';
            line_len = comment - line;
        }

        size_t i = 0;
        while (i < line_len) {
            char c = line[i];
            if (isspace((unsigned char)c)) {
                i++;
                continue;
            }
            if (line[i] == '-' && line[i + 1] == '>') {
                tok_list_push(tokens, TOK_EBNF_ARROW, "->");
                i += 2;
                continue;
            }
            if (c == '{') { tok_list_push(tokens, TOK_EBNF_LBRACE, "{"); i++; continue; }
            if (c == '}') { tok_list_push(tokens, TOK_EBNF_RBRACE, "}"); i++; continue; }
            if (c == '[') { tok_list_push(tokens, TOK_EBNF_LBRACKET, "["); i++; continue; }
            if (c == ']') { tok_list_push(tokens, TOK_EBNF_RBRACKET, "]"); i++; continue; }
            if (c == '(') { tok_list_push(tokens, TOK_EBNF_LPAR, "("); i++; continue; }
            if (c == ')') { tok_list_push(tokens, TOK_EBNF_RPAR, ")"); i++; continue; }
            if (c == '|') { tok_list_push(tokens, TOK_EBNF_PIPE, "|"); i++; continue; }

            if (c == '<') {
                size_t j = i + 1;
                while (j < line_len && line[j] != '>') j++;
                size_t end_j = (j < line_len) ? j + 1 : line_len;
                char buf[256];
                size_t tok_l = end_j - i;
                if (tok_l >= sizeof(buf)) tok_l = sizeof(buf) - 1;
                memcpy(buf, &line[i], tok_l);
                buf[tok_l] = '\0';
                tok_list_push(tokens, TOK_EBNF_NT, buf);
                i = end_j;
                continue;
            }
            if (c == '"') {
                size_t j = i + 1;
                while (j < line_len && line[j] != '"') j++;
                size_t end_j = (j < line_len) ? j + 1 : line_len;
                char buf[256];
                size_t tok_l = end_j - i;
                if (tok_l >= sizeof(buf)) tok_l = sizeof(buf) - 1;
                memcpy(buf, &line[i], tok_l);
                buf[tok_l] = '\0';
                tok_list_push(tokens, TOK_EBNF_T, buf);
                i = end_j;
                continue;
            }
            if (c == '\'') {
                size_t j = i + 1;
                while (j < line_len && line[j] != '\'') j++;
                size_t end_j = (j < line_len) ? j + 1 : line_len;
                char buf[256];
                buf[0] = '"';
                size_t inner_l = (j < line_len) ? j - (i + 1) : line_len - (i + 1);
                if (inner_l >= sizeof(buf) - 2) inner_l = sizeof(buf) - 3;
                memcpy(&buf[1], &line[i + 1], inner_l);
                buf[1 + inner_l] = '"';
                buf[2 + inner_l] = '\0';
                tok_list_push(tokens, TOK_EBNF_T, buf);
                i = end_j;
                continue;
            }
            if (isalnum((unsigned char)c) || c == '_') {
                size_t j = i;
                while (j < line_len && (isalnum((unsigned char)line[j]) || line[j] == '_')) j++;
                char buf[256];
                size_t tok_l = j - i;
                if (tok_l >= sizeof(buf)) tok_l = sizeof(buf) - 1;
                memcpy(buf, &line[i], tok_l);
                buf[tok_l] = '\0';
                tok_list_push(tokens, TOK_EBNF_ID, buf);
                i = j;
                continue;
            }

            char single[2] = {c, '\0'};
            tok_list_push(tokens, TOK_EBNF_T, single);
            i++;
        }
    }
}

/* Parser recursivo de RHS */
typedef struct {
    EbnfToken *tokens;
    size_t count;
    size_t pos;
    const char *orig_lhs;
    AuxMap *aux_map;
    Rule *aux_rules;
    size_t aux_count;
    size_t aux_capacity;
} RhsParser;

static void rhs_add_aux_rule(RhsParser *rp, const char *lhs, AltList alts) {
    if (rp->aux_count >= rp->aux_capacity) {
        size_t new_cap = rp->aux_capacity == 0 ? 8 : rp->aux_capacity * 2;
        rp->aux_rules = (Rule *)realloc(rp->aux_rules, new_cap * sizeof(Rule));
        rp->aux_capacity = new_cap;
    }
    rp->aux_rules[rp->aux_count].lhs = my_strdup(lhs);
    rp->aux_rules[rp->aux_count].alts = alts;
    set_init(&rp->aux_rules[rp->aux_count].first);
    set_init(&rp->aux_rules[rp->aux_count].follow);
    rp->aux_count++;
}

static AltList parse_rhs_expr(RhsParser *rp);

static char *parse_rhs_factor(RhsParser *rp) {
    if (rp->pos >= rp->count) return NULL;
    EbnfToken t = rp->tokens[rp->pos];

    if (t.kind == TOK_EBNF_LBRACE) {
        rp->pos++; /* eat { */
        AltList inner = parse_rhs_expr(rp);
        if (rp->pos < rp->count && rp->tokens[rp->pos].kind == TOK_EBNF_RBRACE) {
            rp->pos++; /* eat } */
        }
        char *aux_nt = make_aux_name(rp->aux_map, rp->orig_lhs);
        AltList aux_alts;
        alts_init(&aux_alts);
        for (size_t i = 0; i < inner.count; i++) {
            SymbolSeq s = seq_clone(&inner.items[i]);
            seq_push(&s, aux_nt);
            alts_push(&aux_alts, s);
        }
        SymbolSeq eps;
        seq_init(&eps);
        alts_push(&aux_alts, eps);

        rhs_add_aux_rule(rp, aux_nt, aux_alts);
        alts_free(&inner);
        return aux_nt;
    } else if (t.kind == TOK_EBNF_LBRACKET) {
        rp->pos++; /* eat [ */
        AltList inner = parse_rhs_expr(rp);
        if (rp->pos < rp->count && rp->tokens[rp->pos].kind == TOK_EBNF_RBRACKET) {
            rp->pos++; /* eat ] */
        }
        char *aux_nt = make_aux_name(rp->aux_map, rp->orig_lhs);
        AltList aux_alts;
        alts_init(&aux_alts);
        for (size_t i = 0; i < inner.count; i++) {
            SymbolSeq s = seq_clone(&inner.items[i]);
            alts_push(&aux_alts, s);
        }
        SymbolSeq eps;
        seq_init(&eps);
        alts_push(&aux_alts, eps);

        rhs_add_aux_rule(rp, aux_nt, aux_alts);
        alts_free(&inner);
        return aux_nt;
    } else if (t.kind == TOK_EBNF_LPAR) {
        rp->pos++; /* eat ( */
        AltList inner = parse_rhs_expr(rp);
        if (rp->pos < rp->count && rp->tokens[rp->pos].kind == TOK_EBNF_RPAR) {
            rp->pos++; /* eat ) */
        }
        char *aux_nt = make_aux_name(rp->aux_map, rp->orig_lhs);
        AltList aux_alts;
        alts_init(&aux_alts);
        for (size_t i = 0; i < inner.count; i++) {
            SymbolSeq s = seq_clone(&inner.items[i]);
            alts_push(&aux_alts, s);
        }
        rhs_add_aux_rule(rp, aux_nt, aux_alts);
        alts_free(&inner);
        return aux_nt;
    } else if (t.kind == TOK_EBNF_NT || t.kind == TOK_EBNF_T || t.kind == TOK_EBNF_ID) {
        rp->pos++;
        return my_strdup(t.val);
    } else {
        rp->pos++;
        return NULL;
    }
}

static SymbolSeq parse_rhs_seq(RhsParser *rp) {
    SymbolSeq seq;
    seq_init(&seq);
    while (rp->pos < rp->count) {
        EbnfTokKind k = rp->tokens[rp->pos].kind;
        if (k == TOK_EBNF_PIPE || k == TOK_EBNF_RBRACE ||
            k == TOK_EBNF_RBRACKET || k == TOK_EBNF_RPAR) {
            break;
        }
        char *f = parse_rhs_factor(rp);
        if (f) {
            seq_push(&seq, f);
            free(f);
        }
    }
    return seq;
}

static AltList parse_rhs_expr(RhsParser *rp) {
    AltList alts;
    alts_init(&alts);
    alts_push(&alts, parse_rhs_seq(rp));

    while (rp->pos < rp->count && rp->tokens[rp->pos].kind == TOK_EBNF_PIPE) {
        rp->pos++; /* eat | */
        alts_push(&alts, parse_rhs_seq(rp));
    }
    return alts;
}

void ll1_grammar_init(LL1Grammar *g) {
    g->rules = NULL;
    g->rule_count = 0;
    g->rule_capacity = 0;
    set_init(&g->non_terminals);
    set_init(&g->nullable);
}

int ll1_grammar_parse(LL1Grammar *g, const char *ebnf_text) {
    ll1_grammar_init(g);
    EbnfTokenList tokens;
    tokenize_ebnf(ebnf_text, &tokens);

    /* Encontrar inicios de regla */
    size_t *rule_starts = NULL;
    size_t start_count = 0;
    for (size_t i = 0; i + 1 < tokens.count; i++) {
        if (tokens.items[i].kind == TOK_EBNF_NT && tokens.items[i + 1].kind == TOK_EBNF_ARROW) {
            rule_starts = (size_t *)realloc(rule_starts, (start_count + 1) * sizeof(size_t));
            rule_starts[start_count++] = i;
        }
    }

    AuxMap aux_map = {NULL, 0, 0};
    RhsParser rp;
    rp.tokens = tokens.items;
    rp.count = tokens.count;
    rp.pos = 0;
    rp.orig_lhs = NULL;
    rp.aux_map = &aux_map;
    rp.aux_rules = NULL;
    rp.aux_count = 0;
    rp.aux_capacity = 0;

    for (size_t idx = 0; idx < start_count; idx++) {
        size_t start_i = rule_starts[idx];
        size_t end_i = (idx + 1 < start_count) ? rule_starts[idx + 1] : tokens.count;
        const char *lhs = tokens.items[start_i].val;

        rp.orig_lhs = lhs;
        rp.pos = start_i + 2;
        rp.count = end_i;

        AltList alts = parse_rhs_expr(&rp);

        /* Buscar si ya existe la regla para combinar alternativas */
        Rule *existing = NULL;
        for (size_t r = 0; r < g->rule_count; r++) {
            if (strcmp(g->rules[r].lhs, lhs) == 0) {
                existing = &g->rules[r];
                break;
            }
        }
        if (existing) {
            for (size_t a = 0; a < alts.count; a++) {
                alts_push(&existing->alts, alts.items[a]);
            }
            free(alts.items);
        } else {
            if (g->rule_count >= g->rule_capacity) {
                size_t new_cap = g->rule_capacity == 0 ? 16 : g->rule_capacity * 2;
                g->rules = (Rule *)realloc(g->rules, new_cap * sizeof(Rule));
                g->rule_capacity = new_cap;
            }
            g->rules[g->rule_count].lhs = my_strdup(lhs);
            g->rules[g->rule_count].alts = alts;
            set_init(&g->rules[g->rule_count].first);
            set_init(&g->rules[g->rule_count].follow);
            g->rule_count++;
        }
    }

    /* Añadir reglas auxiliares generadas */
    for (size_t i = 0; i < rp.aux_count; i++) {
        if (g->rule_count >= g->rule_capacity) {
            size_t new_cap = g->rule_capacity == 0 ? 16 : g->rule_capacity * 2;
            g->rules = (Rule *)realloc(g->rules, new_cap * sizeof(Rule));
            g->rule_capacity = new_cap;
        }
        g->rules[g->rule_count++] = rp.aux_rules[i];
    }
    if (rp.aux_rules) free(rp.aux_rules);

    for (size_t i = 0; i < g->rule_count; i++) {
        set_add(&g->non_terminals, g->rules[i].lhs);
    }

    aux_map_free(&aux_map);
    if (rule_starts) free(rule_starts);
    tok_list_free(&tokens);
    return 1;
}

static Rule *find_rule(LL1Grammar *g, const char *lhs) {
    for (size_t i = 0; i < g->rule_count; i++) {
        if (strcmp(g->rules[i].lhs, lhs) == 0) {
            return &g->rules[i];
        }
    }
    return NULL;
}

static int is_sequence_nullable(LL1Grammar *g, const char *const *symbols, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (!set_contains(&g->nullable, symbols[i])) {
            return 0;
        }
    }
    return 1;
}

static StringSet first_of_sequence(LL1Grammar *g, const char *const *symbols, size_t count) {
    StringSet res;
    set_init(&res);
    for (size_t i = 0; i < count; i++) {
        const char *sym = symbols[i];
        if (set_contains(&g->non_terminals, sym)) {
            Rule *r = find_rule(g, sym);
            if (r) {
                set_union(&res, &r->first);
            }
            if (!set_contains(&g->nullable, sym)) {
                break;
            }
        } else {
            set_add(&res, sym);
            break;
        }
    }
    return res;
}

void ll1_grammar_calculate(LL1Grammar *g) {
    /* 1. NULLABLE */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (size_t i = 0; i < g->rule_count; i++) {
            Rule *r = &g->rules[i];
            if (set_contains(&g->nullable, r->lhs)) continue;
            for (size_t a = 0; a < r->alts.count; a++) {
                SymbolSeq *seq = &r->alts.items[a];
                if (is_sequence_nullable(g, (const char *const *)seq->symbols, seq->count)) {
                    set_add(&g->nullable, r->lhs);
                    changed = 1;
                    break;
                }
            }
        }
    }

    /* 2. FIRST */
    changed = 1;
    while (changed) {
        changed = 0;
        for (size_t i = 0; i < g->rule_count; i++) {
            Rule *r = &g->rules[i];
            for (size_t a = 0; a < r->alts.count; a++) {
                SymbolSeq *seq = &r->alts.items[a];
                StringSet f = first_of_sequence(g, (const char *const *)seq->symbols, seq->count);
                if (!set_is_subset(&f, &r->first)) {
                    set_union(&r->first, &f);
                    changed = 1;
                }
                set_free(&f);
            }
        }
    }

    /* 3. FOLLOW */
    if (g->rule_count > 0) {
        set_add(&g->rules[0].follow, "$");
    }
    changed = 1;
    while (changed) {
        changed = 0;
        for (size_t i = 0; i < g->rule_count; i++) {
            Rule *r = &g->rules[i];
            for (size_t a = 0; a < r->alts.count; a++) {
                SymbolSeq *seq = &r->alts.items[a];
                for (size_t k = 0; k < seq->count; k++) {
                    const char *sym = seq->symbols[k];
                    if (set_contains(&g->non_terminals, sym)) {
                        Rule *sym_rule = find_rule(g, sym);
                        if (sym_rule) {
                            const char *const *beta = (const char *const *)&seq->symbols[k + 1];
                            size_t beta_count = seq->count - (k + 1);

                            StringSet f_beta = first_of_sequence(g, beta, beta_count);
                            if (!set_is_subset(&f_beta, &sym_rule->follow)) {
                                set_union(&sym_rule->follow, &f_beta);
                                changed = 1;
                            }
                            set_free(&f_beta);

                            if (is_sequence_nullable(g, beta, beta_count)) {
                                if (!set_is_subset(&r->follow, &sym_rule->follow)) {
                                    set_union(&sym_rule->follow, &r->follow);
                                    changed = 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void ll1_conflicts_init(LL1ConflictList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void ll1_conflicts_free(LL1ConflictList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].non_terminal) free(list->items[i].non_terminal);
        set_free(&list->items[i].intersection);
    }
    if (list->items) free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void ll1_grammar_find_conflicts(LL1Grammar *g, LL1ConflictList *confs) {
    ll1_conflicts_init(confs);

    for (size_t r = 0; r < g->rule_count; r++) {
        Rule *rule = &g->rules[r];
        size_t n_alts = rule->alts.count;
        if (n_alts < 2) continue;

        StringSet *sel_sets = (StringSet *)malloc(n_alts * sizeof(StringSet));
        for (size_t a = 0; a < n_alts; a++) {
            SymbolSeq *seq = &rule->alts.items[a];
            sel_sets[a] = first_of_sequence(g, (const char *const *)seq->symbols, seq->count);
            if (is_sequence_nullable(g, (const char *const *)seq->symbols, seq->count)) {
                set_union(&sel_sets[a], &rule->follow);
            }
        }

        for (size_t i = 0; i < n_alts; i++) {
            for (size_t j = i + 1; j < n_alts; j++) {
                StringSet inter;
                set_intersect(&inter, &sel_sets[i], &sel_sets[j]);
                if (inter.count > 0) {
                    if (confs->count >= confs->capacity) {
                        size_t new_cap = confs->capacity == 0 ? 4 : confs->capacity * 2;
                        confs->items = (LL1Conflict *)realloc(confs->items, new_cap * sizeof(LL1Conflict));
                        confs->capacity = new_cap;
                    }
                    confs->items[confs->count].non_terminal = my_strdup(rule->lhs);
                    confs->items[confs->count].alt1 = i;
                    confs->items[confs->count].alt2 = j;
                    confs->items[confs->count].intersection = inter;
                    confs->count++;
                } else {
                    set_free(&inter);
                }
            }
        }

        for (size_t a = 0; a < n_alts; a++) {
            set_free(&sel_sets[a]);
        }
        free(sel_sets);
    }
}

void ll1_grammar_free(LL1Grammar *g) {
    if (!g) return;
    for (size_t i = 0; i < g->rule_count; i++) {
        free(g->rules[i].lhs);
        alts_free(&g->rules[i].alts);
        set_free(&g->rules[i].first);
        set_free(&g->rules[i].follow);
    }
    if (g->rules) free(g->rules);
    g->rules = NULL;
    g->rule_count = 0;
    g->rule_capacity = 0;
    set_free(&g->non_terminals);
    set_free(&g->nullable);
}
