#ifndef TOY_PARSER_H
#define TOY_PARSER_H

#include "lexer.h"
#include "ast.h"
#include "arena.h"
#include <setjmp.h>

typedef struct {
    Lexer *lexer;
    Token la;
    Arena arena;
    jmp_buf jmp_env;
    int has_error;
    char error_msg[512];
} Parser;

void parser_init(Parser *p, Lexer *lexer);
ASTProgram *parser_parse(Parser *p);
void parser_destroy(Parser *p);

#endif /* TOY_PARSER_H */
