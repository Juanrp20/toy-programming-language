#include "token.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *word;
    TokenKind kind;
} KeywordMap;

static const KeywordMap KEYWORDS[] = {
    {"and", TOK_AND},
    {"bool", TOK_KW_BOOL},
    {"do", TOK_DO},
    {"else", TOK_ELSE},
    {"end", TOK_END},
    {"false", TOK_FALSE},
    {"for", TOK_FOR},
    {"func", TOK_FUNC},
    {"if", TOK_IF},
    {"local", TOK_LOCAL},
    {"main", TOK_MAIN},
    {"number", TOK_KW_NUMBER},
    {"or", TOK_OR},
    {"print", TOK_PRINT},
    {"println", TOK_PRINTLN},
    {"ret", TOK_RET},
    {"string", TOK_KW_STRING},
    {"then", TOK_THEN},
    {"true", TOK_TRUE},
    {"while", TOK_WHILE},
    {NULL, TOK_ID}
};

const char *token_kind_to_str(TokenKind kind) {
    switch (kind) {
        case TOK_EOF:       return "TOK-EOF";
        case TOK_ERROR:     return "TOK-ERROR";
        case TOK_IF:        return "TOK-IF";
        case TOK_THEN:      return "TOK-THEN";
        case TOK_ELSE:      return "TOK-ELSE";
        case TOK_TRUE:      return "TOK-TRUE";
        case TOK_FALSE:     return "TOK-FALSE";
        case TOK_AND:       return "TOK-AND";
        case TOK_OR:        return "TOK-OR";
        case TOK_WHILE:     return "TOK-WHILE";
        case TOK_DO:        return "TOK-DO";
        case TOK_FOR:       return "TOK-FOR";
        case TOK_FUNC:      return "TOK-FUNC";
        case TOK_END:       return "TOK-END";
        case TOK_MAIN:      return "TOK-MAIN";
        case TOK_LOCAL:     return "TOK-LOCAL";
        case TOK_PRINT:     return "TOK-PRINT";
        case TOK_PRINTLN:   return "TOK-PRINTLN";
        case TOK_RET:       return "TOK-RET";
        case TOK_KW_NUMBER: return "TOK-KW-NUMBER";
        case TOK_KW_STRING: return "TOK-KW-STRING";
        case TOK_KW_BOOL:   return "TOK-KW-BOOL";
        case TOK_MINUS:     return "TOK-MINUS";
        case TOK_PLUS:      return "TOK-PLUS";
        case TOK_MUL:       return "TOK-MUL";
        case TOK_DIV:       return "TOK-DIV";
        case TOK_MOD:       return "TOK-MOD";
        case TOK_EXP:       return "TOK-EXP";
        case TOK_LPAR:      return "TOK-LPAR";
        case TOK_RPAR:      return "TOK-RPAR";
        case TOK_COMMA:     return "TOK-COMMA";
        case TOK_GTE:       return "TOK-GTE";
        case TOK_GT:        return "TOK-GT";
        case TOK_LTE:       return "TOK-LTE";
        case TOK_LT:        return "TOK-LT";
        case TOK_EQ:        return "TOK-EQ";
        case TOK_NEQ:       return "TOK-NEQ";
        case TOK_NOT:       return "TOK-NOT";
        case TOK_ASSIGN:    return "TOK-ASSIGN";
        case TOK_ID:        return "TOK-ID";
        case TOK_NUMBER:    return "TOK-NUMBER";
        case TOK_STRING:    return "TOK-STRING";
        default:            return "TOK-UNKNOWN";
    }
}

Token token_create(TokenKind kind, const char *lexeme, int line, int col) {
    Token tok;
    tok.kind = kind;
    tok.line = line;
    tok.col = col;
    if (lexeme) {
        size_t len = strlen(lexeme);
        tok.lexeme = (char *)malloc(len + 1);
        if (tok.lexeme) {
            memcpy(tok.lexeme, lexeme, len + 1);
        }
    } else {
        tok.lexeme = (char *)malloc(1);
        if (tok.lexeme) {
            tok.lexeme[0] = '\0';
        }
    }
    return tok;
}

void token_free(Token *tok) {
    if (tok && tok->lexeme) {
        free(tok->lexeme);
        tok->lexeme = NULL;
    }
}

TokenKind token_lookup_keyword(const char *ident) {
    for (int i = 0; KEYWORDS[i].word != NULL; i++) {
        if (strcmp(KEYWORDS[i].word, ident) == 0) {
            return KEYWORDS[i].kind;
        }
    }
    return TOK_ID;
}
