#ifndef TOY_LEXER_H
#define TOY_LEXER_H

#include "token.h"
#include <stddef.h>

typedef struct {
    const char *texto;
    size_t i;
    size_t n;
    int line;
    int col;
    int has_error;
    char *error_msg;
    int error_line;
    int error_col;
} Lexer;

void lexer_init(Lexer *l, const char *texto);
Token lexer_siguiente(Lexer *l);
void lexer_destroy(Lexer *l);

#endif /* TOY_LEXER_H */
