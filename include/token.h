#ifndef TOY_TOKEN_H
#define TOY_TOKEN_H

#include <stddef.h>

typedef enum {
    TOK_EOF = 0,
    TOK_ERROR,

    /* Palabras reservadas */
    TOK_IF,
    TOK_THEN,
    TOK_ELSE,
    TOK_TRUE,
    TOK_FALSE,
    TOK_AND,
    TOK_OR,
    TOK_WHILE,
    TOK_DO,
    TOK_FOR,
    TOK_FUNC,
    TOK_END,
    TOK_MAIN,
    TOK_LOCAL,
    TOK_PRINT,
    TOK_PRINTLN,
    TOK_RET,
    TOK_KW_NUMBER,
    TOK_KW_STRING,
    TOK_KW_BOOL,

    /* Operadores y puntuación */
    TOK_MINUS,
    TOK_PLUS,
    TOK_MUL,
    TOK_DIV,
    TOK_MOD,
    TOK_EXP,
    TOK_LPAR,
    TOK_RPAR,
    TOK_COMMA,
    TOK_GTE,
    TOK_GT,
    TOK_LTE,
    TOK_LT,
    TOK_EQ,
    TOK_NEQ,
    TOK_NOT,
    TOK_ASSIGN,

    /* Literales e identificadores */
    TOK_ID,
    TOK_NUMBER,
    TOK_STRING
} TokenKind;

typedef struct {
    TokenKind kind;
    char *lexeme;
    int line;
    int col;
} Token;

const char *token_kind_to_str(TokenKind kind);
Token token_create(TokenKind kind, const char *lexeme, int line, int col);
void token_free(Token *tok);
TokenKind token_lookup_keyword(const char *ident);

#endif /* TOY_TOKEN_H */
