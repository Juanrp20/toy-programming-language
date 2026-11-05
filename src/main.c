#include "lexer.h"
#include "parser.h"
#include "ast.h"
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

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: %s archivo.toy [--tokens] [--ast]\n", argv[0]);
        return 1;
    }

    const char *filename = NULL;
    int opt_tokens = 0;
    int opt_ast = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) {
            opt_tokens = 1;
        } else if (strcmp(argv[i], "--ast") == 0) {
            opt_ast = 1;
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        }
    }

    if (!filename) {
        printf("Uso: %s archivo.toy [--tokens] [--ast]\n", argv[0]);
        return 1;
    }

    char *source = read_entire_file(filename);
    if (!source) {
        return 1;
    }

    Lexer lexer;
    lexer_init(&lexer, source);

    if (opt_tokens) {
        while (1) {
            Token tok = lexer_siguiente(&lexer);
            if (tok.kind == TOK_ERROR) {
                printf("Línea %d, columna %d: %s\n", tok.line, tok.col, tok.lexeme);
                token_free(&tok);
                lexer_destroy(&lexer);
                free(source);
                return 1;
            }
            printf("%s %s %d:%d\n", token_kind_to_str(tok.kind), tok.lexeme, tok.line, tok.col);
            int is_eof = (tok.kind == TOK_EOF);
            token_free(&tok);
            if (is_eof) break;
        }
        lexer_destroy(&lexer);
        free(source);
        return 0;
    }

    Parser parser;
    parser_init(&parser, &lexer);
    ASTProgram *ast = parser_parse(&parser);

    if (!ast || parser.has_error) {
        printf("%s\n", parser.error_msg);
        parser_destroy(&parser);
        lexer_destroy(&lexer);
        free(source);
        return 1;
    }

    if (opt_ast) {
        ast_print_program(ast, stdout);
    } else {
        printf("OK\n");
    }

    parser_destroy(&parser);
    lexer_destroy(&lexer);
    free(source);
    return 0;
}
