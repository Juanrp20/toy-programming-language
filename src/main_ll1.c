#include "ll1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_entire_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("Error al abrir archivo");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';
    fclose(f);
    return buf;
}

static void print_set_python_style(const StringSet *s) {
    printf("[");
    for (size_t i = 0; i < s->count; i++) {
        if (i > 0) printf(", ");
        printf("'%s'", s->items[i]);
    }
    printf("]");
}

static int compare_strings(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: %s gramatica.txt [--sets]\n", argv[0]);
        return 1;
    }

    const char *filename = NULL;
    int opt_sets = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--sets") == 0) {
            opt_sets = 1;
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        }
    }

    if (!filename) {
        printf("Uso: %s gramatica.txt [--sets]\n", argv[0]);
        return 1;
    }

    char *text = read_entire_file(filename);
    if (!text) {
        return 1;
    }

    LL1Grammar g;
    ll1_grammar_parse(&g, text);
    ll1_grammar_calculate(&g);

    LL1ConflictList confs;
    ll1_grammar_find_conflicts(&g, &confs);

    if (confs.count > 0) {
        for (size_t i = 0; i < confs.count; i++) {
            printf("Conflicto en %s: alternativa %zu y %zu comparten ",
                   confs.items[i].non_terminal,
                   confs.items[i].alt1 + 1,
                   confs.items[i].alt2 + 1);
            print_set_python_style(&confs.items[i].intersection);
            printf("\n");
        }
    } else {
        printf("ninguno\n");
    }

    if (opt_sets) {
        /* Ordenar no terminales alfabéticamente */
        char **sorted_nt = (char **)malloc(g.rule_count * sizeof(char *));
        for (size_t i = 0; i < g.rule_count; i++) {
            sorted_nt[i] = g.rules[i].lhs;
        }
        qsort(sorted_nt, g.rule_count, sizeof(char *), compare_strings);

        for (size_t i = 0; i < g.rule_count; i++) {
            const char *nt = sorted_nt[i];
            if (nt[0] == '<') {
                /* Buscar regla */
                for (size_t r = 0; r < g.rule_count; r++) {
                    if (strcmp(g.rules[r].lhs, nt) == 0) {
                        printf("PRIMERO(%s) = ", nt);
                        print_set_python_style(&g.rules[r].first);
                        printf("\n");
                        printf("SIGUIENTE(%s) = ", nt);
                        print_set_python_style(&g.rules[r].follow);
                        printf("\n");
                        break;
                    }
                }
            }
        }
        free(sorted_nt);
    }

    ll1_conflicts_free(&confs);
    ll1_grammar_free(&g);
    free(text);
    return 0;
}
