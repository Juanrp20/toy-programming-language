#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} StrBuf;

static void buf_init(StrBuf *b) {
    b->cap = 32;
    b->len = 0;
    b->data = (char *)malloc(b->cap);
    if (b->data) {
        b->data[0] = '\0';
    }
}

static void buf_push(StrBuf *b, char c) {
    if (b->len + 2 > b->cap) {
        b->cap *= 2;
        b->data = (char *)realloc(b->data, b->cap);
    }
    b->data[b->len++] = c;
    b->data[b->len] = '\0';
}

static void buf_free(StrBuf *b) {
    if (b->data) {
        free(b->data);
        b->data = NULL;
    }
    b->len = 0;
    b->cap = 0;
}

void lexer_init(Lexer *l, const char *texto) {
    l->texto = texto ? texto : "";
    l->i = 0;
    l->n = strlen(l->texto);
    l->line = 1;
    l->col = 1;
    l->has_error = 0;
    l->error_msg = NULL;
    l->error_line = 0;
    l->error_col = 0;
}

void lexer_destroy(Lexer *l) {
    if (l->error_msg) {
        free(l->error_msg);
        l->error_msg = NULL;
    }
}

static int char_at(Lexer *l) {
    if (l->i >= l->n) {
        return -1;
    }
    return (unsigned char)l->texto[l->i];
}

static int avanzar(Lexer *l) {
    int c = char_at(l);
    if (c == -1) {
        l->i++;
        return -1;
    }
    l->i++;
    if (c == '\n') {
        l->line++;
        l->col = 1;
    } else {
        l->col++;
    }
    return c;
}

static int es_letra(int c) {
    if (c == -1) return 0;
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

static int es_digito(int c) {
    if (c == -1) return 0;
    return (c >= '0' && c <= '9');
}

static Token error_token(Lexer *l, int line, int col, const char *msg) {
    l->has_error = 1;
    l->error_line = line;
    l->error_col = col;
    if (l->error_msg) free(l->error_msg);
    l->error_msg = (char *)malloc(strlen(msg) + 1);
    if (l->error_msg) {
        strcpy(l->error_msg, msg);
    }
    return token_create(TOK_ERROR, msg, line, col);
}

static Token leer_identificador(Lexer *l) {
    int start_line = l->line;
    int start_col = l->col;
    StrBuf buf;
    buf_init(&buf);

    while (1) {
        int c = char_at(l);
        if (c != -1 && (es_letra(c) || es_digito(c))) {
            buf_push(&buf, (char)c);
            avanzar(l);
        } else {
            break;
        }
    }

    TokenKind kind = token_lookup_keyword(buf.data);
    Token tok = token_create(kind, buf.data, start_line, start_col);
    buf_free(&buf);
    return tok;
}

static Token leer_numero(Lexer *l) {
    int start_line = l->line;
    int start_col = l->col;
    StrBuf buf;
    buf_init(&buf);

    while (1) {
        int c = char_at(l);
        if (c != -1 && es_digito(c)) {
            buf_push(&buf, (char)c);
            avanzar(l);
        } else {
            break;
        }
    }

    int c = char_at(l);
    if (c == '.') {
        avanzar(l);
        int c2 = char_at(l);
        if (c2 != -1 && es_digito(c2)) {
            buf_push(&buf, '.');
            while (1) {
                int c3 = char_at(l);
                if (c3 != -1 && es_digito(c3)) {
                    buf_push(&buf, (char)c3);
                    avanzar(l);
                } else {
                    break;
                }
            }
            Token tok = token_create(TOK_NUMBER, buf.data, start_line, start_col);
            buf_free(&buf);
            return tok;
        } else {
            buf_free(&buf);
            return error_token(l, start_line, start_col, "número mal formado");
        }
    }

    Token tok = token_create(TOK_NUMBER, buf.data, start_line, start_col);
    buf_free(&buf);
    return tok;
}

static Token leer_cadena(Lexer *l, char comilla) {
    int start_line = l->line;
    int start_col = l->col;
    avanzar(l); /* consumir comilla inicial */

    StrBuf buf;
    buf_init(&buf);

    while (1) {
        int c = char_at(l);
        if (c == -1 || c == '\n') {
            buf_free(&buf);
            return error_token(l, start_line, start_col, "cadena sin cerrar");
        }
        if (c == comilla) {
            avanzar(l); /* consumir comilla final */
            break;
        }
        buf_push(&buf, (char)c);
        avanzar(l);
    }

    Token tok = token_create(TOK_STRING, buf.data, start_line, start_col);
    buf_free(&buf);
    return tok;
}

Token lexer_siguiente(Lexer *l) {
    while (1) {
        int c = char_at(l);
        if (c == -1) {
            return token_create(TOK_EOF, "", l->line, l->col);
        }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f') {
            avanzar(l);
            continue;
        }
        if (c == '\n') {
            avanzar(l);
            continue;
        }
        if (c == '-') {
            int start_line = l->line;
            int start_col = l->col;
            avanzar(l);
            int c2 = char_at(l);
            if (c2 == '-') {
                avanzar(l);
                while (1) {
                    int cc = char_at(l);
                    if (cc == -1 || cc == '\n') {
                        break;
                    }
                    avanzar(l);
                }
                continue;
            } else {
                return token_create(TOK_MINUS, "-", start_line, start_col);
            }
        }
        if (c == '+') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            return token_create(TOK_PLUS, "+", start_line, start_col);
        }
        if (c == '*') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            return token_create(TOK_MUL, "*", start_line, start_col);
        }
        if (c == '/') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            return token_create(TOK_DIV, "/", start_line, start_col);
        }
        if (c == '%') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            return token_create(TOK_MOD, "%", start_line, start_col);
        }
        if (c == '^') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            return token_create(TOK_EXP, "^", start_line, start_col);
        }
        if (c == '(') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            return token_create(TOK_LPAR, "(", start_line, start_col);
        }
        if (c == ')') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            return token_create(TOK_RPAR, ")", start_line, start_col);
        }
        if (c == ',') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            return token_create(TOK_COMMA, ",", start_line, start_col);
        }
        if (c == '.') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            return error_token(l, start_line, start_col, "carácter '.' no válido");
        }
        if (c == '>') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            int c2 = char_at(l);
            if (c2 == '=') {
                avanzar(l);
                return token_create(TOK_GTE, ">=", start_line, start_col);
            }
            return token_create(TOK_GT, ">", start_line, start_col);
        }
        if (c == '<') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            int c2 = char_at(l);
            if (c2 == '=') {
                avanzar(l);
                return token_create(TOK_LTE, "<=", start_line, start_col);
            }
            return token_create(TOK_LT, "<", start_line, start_col);
        }
        if (c == '=') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            int c2 = char_at(l);
            if (c2 == '=') {
                avanzar(l);
                return token_create(TOK_EQ, "==", start_line, start_col);
            }
            return error_token(l, start_line, start_col, "'=' solo no es válido, use '==' o ':='");
        }
        if (c == '~') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            int c2 = char_at(l);
            if (c2 == '=') {
                avanzar(l);
                return token_create(TOK_NEQ, "~=", start_line, start_col);
            }
            return token_create(TOK_NOT, "~", start_line, start_col);
        }
        if (c == ':') {
            int start_line = l->line, start_col = l->col;
            avanzar(l);
            int c2 = char_at(l);
            if (c2 == '=') {
                avanzar(l);
                return token_create(TOK_ASSIGN, ":=", start_line, start_col);
            }
            return error_token(l, start_line, start_col, "':' solo no es válido, use ':='");
        }
        if (c == '"') {
            return leer_cadena(l, '"');
        }
        if (c == '\'') {
            return leer_cadena(l, '\'');
        }
        if (es_letra(c)) {
            return leer_identificador(l);
        }
        if (es_digito(c)) {
            return leer_numero(l);
        }

        int start_line = l->line, start_col = l->col;
        avanzar(l);
        char err[64];
        snprintf(err, sizeof(err), "carácter '%c' no válido", (char)c);
        return error_token(l, start_line, start_col, err);
    }
}
