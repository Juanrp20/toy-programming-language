#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void parser_error(Parser *p, const char *esperado) {
    p->has_error = 1;
    if (p->la.kind == TOK_ERROR) {
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "Línea %d, columna %d: %s",
                 p->la.line, p->la.col, p->la.lexeme ? p->la.lexeme : "");
    } else {
        const char *encontrado = (p->la.lexeme && p->la.lexeme[0] != '\0')
                                 ? p->la.lexeme
                                 : token_kind_to_str(p->la.kind);
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "Línea %d, columna %d: se esperaba %s, se encontró %s",
                 p->la.line, p->la.col, esperado, encontrado);
    }
    longjmp(p->jmp_env, 1);
}

static Token parser_comer(Parser *p, TokenKind kind) {
    if (p->la.kind == TOK_ERROR) {
        p->has_error = 1;
        snprintf(p->error_msg, sizeof(p->error_msg),
                 "Línea %d, columna %d: %s",
                 p->la.line, p->la.col, p->la.lexeme ? p->la.lexeme : "");
        longjmp(p->jmp_env, 1);
    }
    if (kind != (TokenKind)-1 && p->la.kind != kind) {
        parser_error(p, token_kind_to_str(kind));
    }
    Token tok = p->la;
    p->la = lexer_siguiente(p->lexer);
    return tok;
}

static void parser_eat(Parser *p, TokenKind kind) {
    Token tok = parser_comer(p, kind);
    token_free(&tok);
}

static char *parser_eat_id(Parser *p) {
    Token tok = parser_comer(p, TOK_ID);
    char *id = arena_strdup(&p->arena, tok.lexeme);
    token_free(&tok);
    return id;
}

static int es_statement_start(TokenKind k) {
    return (k == TOK_IF || k == TOK_WHILE || k == TOK_FOR ||
            k == TOK_PRINT || k == TOK_PRINTLN || k == TOK_LOCAL ||
            k == TOK_ID);
}

static int es_expr_start(TokenKind k) {
    return (k == TOK_NUMBER || k == TOK_STRING || k == TOK_TRUE ||
            k == TOK_FALSE || k == TOK_LPAR || k == TOK_ID ||
            k == TOK_MINUS || k == TOK_NOT);
}

/* Declaraciones adelantadas */
static ASTProgram *parse_program(Parser *p);
static ASTFuncList parse_func_rest(Parser *p);
static ASTStringList parse_param_list(Parser *p);
static ASTBlock *parse_block(Parser *p);
static ASTStmt *parse_statement(Parser *p);
static ASTStmt *parse_if_stmt(Parser *p);
static ASTStmt *parse_while_stmt(Parser *p);
static ASTStmt *parse_for_stmt(Parser *p);
static ASTStmt *parse_print_stmt(Parser *p);
static ASTStmt *parse_local_stmt(Parser *p);
static ASTStmt *parse_ret_stmt(Parser *p);
static ASTStmt *parse_id_stmt(Parser *p);
static ASTExprList parse_arg_list(Parser *p);

static ASTExpr *parse_expr(Parser *p);
static ASTExpr *parse_or_expr(Parser *p);
static ASTExpr *parse_and_expr(Parser *p);
static ASTExpr *parse_eq_expr(Parser *p);
static ASTExpr *parse_rel_expr(Parser *p);
static ASTExpr *parse_add_expr(Parser *p);
static ASTExpr *parse_mult_expr(Parser *p);
static ASTExpr *parse_mod_expr(Parser *p);
static ASTExpr *parse_unary_expr(Parser *p);
static ASTExpr *parse_exp_expr(Parser *p);
static ASTExpr *parse_primary(Parser *p);

void parser_init(Parser *p, Lexer *lexer) {
    p->lexer = lexer;
    p->has_error = 0;
    p->error_msg[0] = '\0';
    arena_init(&p->arena);
    p->la.kind = TOK_EOF;
    p->la.lexeme = NULL;
    p->la.line = 1;
    p->la.col = 1;
}

void parser_destroy(Parser *p) {
    token_free(&p->la);
    arena_free(&p->arena);
}

ASTProgram *parser_parse(Parser *p) {
    if (setjmp(p->jmp_env) != 0) {
        return NULL;
    }
    p->la = lexer_siguiente(p->lexer);
    if (p->la.kind == TOK_ERROR) {
        parser_error(p, "");
    }
    ASTProgram *ast = parse_program(p);
    if (p->la.kind != TOK_EOF) {
        parser_error(p, "TOK-EOF");
    }
    return ast;
}

static ASTProgram *parse_program(Parser *p) {
    parser_eat(p, TOK_FUNC);
    ASTFuncList funcs = parse_func_rest(p);
    return ast_program_create(&p->arena, funcs);
}

static ASTFuncList parse_func_rest(Parser *p) {
    ASTFuncList funcs;
    ast_func_list_init(&p->arena, &funcs);

    if (p->la.kind == TOK_MAIN) {
        parser_eat(p, TOK_MAIN);
        parser_eat(p, TOK_LPAR);
        parser_eat(p, TOK_RPAR);
        ASTBlock *bloque = parse_block(p);
        parser_eat(p, TOK_END);
        parser_eat(p, TOK_MAIN);
        parser_eat(p, TOK_LPAR);
        parser_eat(p, TOK_RPAR);

        ASTFunc *main_fn = ast_main_create(&p->arena, bloque);
        ast_func_list_push(&p->arena, &funcs, main_fn);
        return funcs;
    } else if (p->la.kind == TOK_ID) {
        char *nombre = parser_eat_id(p);
        parser_eat(p, TOK_LPAR);
        ASTStringList params;
        ast_string_list_init(&p->arena, &params);
        if (p->la.kind == TOK_ID) {
            params = parse_param_list(p);
        }
        parser_eat(p, TOK_RPAR);
        ASTBlock *bloque = parse_block(p);
        parser_eat(p, TOK_END);

        ASTProgram *resto = parse_program(p);
        ASTFunc *fn = ast_func_create(&p->arena, nombre, params, bloque);
        ast_func_list_push(&p->arena, &funcs, fn);

        for (size_t i = 0; i < resto->funcs.count; i++) {
            ast_func_list_push(&p->arena, &funcs, resto->funcs.items[i]);
        }
        return funcs;
    } else {
        parser_error(p, "'main' o ID");
        return funcs;
    }
}

static ASTStringList parse_param_list(Parser *p) {
    ASTStringList params;
    ast_string_list_init(&p->arena, &params);
    ast_string_list_push(&p->arena, &params, parser_eat_id(p));
    while (p->la.kind == TOK_COMMA) {
        parser_eat(p, TOK_COMMA);
        ast_string_list_push(&p->arena, &params, parser_eat_id(p));
    }
    return params;
}

static ASTBlock *parse_block(Parser *p) {
    ASTStmtList stmts;
    ast_stmt_list_init(&p->arena, &stmts);

    while (1) {
        if (p->la.kind == TOK_RET) {
            break;
        }
        if (es_statement_start(p->la.kind)) {
            ast_stmt_list_push(&p->arena, &stmts, parse_statement(p));
        } else {
            break;
        }
    }

    ASTStmt *ret = NULL;
    if (p->la.kind == TOK_RET) {
        ret = parse_ret_stmt(p);
    }
    return ast_block_create(&p->arena, stmts, ret);
}

static ASTStmt *parse_statement(Parser *p) {
    TokenKind k = p->la.kind;
    if (k == TOK_IF) return parse_if_stmt(p);
    if (k == TOK_WHILE) return parse_while_stmt(p);
    if (k == TOK_FOR) return parse_for_stmt(p);
    if (k == TOK_PRINT || k == TOK_PRINTLN) return parse_print_stmt(p);
    if (k == TOK_LOCAL) return parse_local_stmt(p);
    if (k == TOK_ID) return parse_id_stmt(p);

    parser_error(p, "sentencia");
    return NULL;
}

static ASTStmt *parse_if_stmt(Parser *p) {
    parser_eat(p, TOK_IF);
    ASTExpr *cond = parse_expr(p);
    parser_eat(p, TOK_THEN);
    ASTBlock *bloque1 = parse_block(p);
    ASTBlock *bloque2 = NULL;
    if (p->la.kind == TOK_ELSE) {
        parser_eat(p, TOK_ELSE);
        bloque2 = parse_block(p);
    }
    parser_eat(p, TOK_END);
    return ast_stmt_if(&p->arena, cond, bloque1, bloque2);
}

static ASTStmt *parse_while_stmt(Parser *p) {
    parser_eat(p, TOK_WHILE);
    ASTExpr *cond = parse_expr(p);
    parser_eat(p, TOK_DO);
    ASTBlock *bloque = parse_block(p);
    parser_eat(p, TOK_END);
    return ast_stmt_while(&p->arena, cond, bloque);
}

static ASTStmt *parse_for_stmt(Parser *p) {
    parser_eat(p, TOK_FOR);
    char *var = parser_eat_id(p);
    parser_eat(p, TOK_ASSIGN);
    ASTExpr *e1 = parse_expr(p);
    parser_eat(p, TOK_COMMA);
    if (p->la.kind == TOK_COMMA) {
        parser_error(p, "segunda expresión");
    }
    ASTExpr *e2 = parse_expr(p);
    ASTExpr *e3 = NULL;
    if (p->la.kind == TOK_COMMA) {
        parser_eat(p, TOK_COMMA);
        if (p->la.kind == TOK_DO) {
            parser_error(p, "segunda expresión");
        }
        e3 = parse_expr(p);
    }
    parser_eat(p, TOK_DO);
    ASTBlock *bloque = parse_block(p);
    parser_eat(p, TOK_END);
    return ast_stmt_for(&p->arena, var, e1, e2, e3, bloque);
}

static ASTStmt *parse_print_stmt(Parser *p) {
    int is_println = (p->la.kind == TOK_PRINTLN);
    if (is_println) {
        parser_eat(p, TOK_PRINTLN);
    } else {
        parser_eat(p, TOK_PRINT);
    }
    ASTExpr *e = parse_expr(p);
    return ast_stmt_print(&p->arena, is_println, e);
}

static ASTStmt *parse_local_stmt(Parser *p) {
    parser_eat(p, TOK_LOCAL);
    char *var = parser_eat_id(p);
    parser_eat(p, TOK_ASSIGN);
    ASTExpr *e = parse_expr(p);
    return ast_stmt_local(&p->arena, var, e);
}

static ASTStmt *parse_ret_stmt(Parser *p) {
    parser_eat(p, TOK_RET);
    ASTExpr *e = NULL;
    if (es_expr_start(p->la.kind)) {
        e = parse_expr(p);
    }
    return ast_stmt_ret(&p->arena, e);
}

static ASTStmt *parse_id_stmt(Parser *p) {
    char *nombre = parser_eat_id(p);
    if (p->la.kind == TOK_ASSIGN) {
        parser_eat(p, TOK_ASSIGN);
        ASTExpr *e = parse_expr(p);
        return ast_stmt_assign(&p->arena, nombre, e);
    } else if (p->la.kind == TOK_LPAR) {
        parser_eat(p, TOK_LPAR);
        ASTExprList args;
        ast_expr_list_init(&p->arena, &args);
        if (es_expr_start(p->la.kind)) {
            args = parse_arg_list(p);
        }
        parser_eat(p, TOK_RPAR);
        return ast_stmt_call(&p->arena, nombre, args);
    } else {
        parser_error(p, "':=' o '('");
        return NULL;
    }
}

static ASTExprList parse_arg_list(Parser *p) {
    ASTExprList args;
    ast_expr_list_init(&p->arena, &args);
    ast_expr_list_push(&p->arena, &args, parse_expr(p));
    while (p->la.kind == TOK_COMMA) {
        parser_eat(p, TOK_COMMA);
        ast_expr_list_push(&p->arena, &args, parse_expr(p));
    }
    return args;
}

static ASTExpr *parse_expr(Parser *p) {
    return parse_or_expr(p);
}

static ASTExpr *parse_or_expr(Parser *p) {
    ASTExpr *node = parse_and_expr(p);
    while (p->la.kind == TOK_OR) {
        parser_eat(p, TOK_OR);
        ASTExpr *right = parse_and_expr(p);
        node = ast_expr_binary(&p->arena, "or", node, right);
    }
    return node;
}

static ASTExpr *parse_and_expr(Parser *p) {
    ASTExpr *node = parse_eq_expr(p);
    while (p->la.kind == TOK_AND) {
        parser_eat(p, TOK_AND);
        ASTExpr *right = parse_eq_expr(p);
        node = ast_expr_binary(&p->arena, "and", node, right);
    }
    return node;
}

static ASTExpr *parse_eq_expr(Parser *p) {
    ASTExpr *node = parse_rel_expr(p);
    while (p->la.kind == TOK_EQ || p->la.kind == TOK_NEQ) {
        TokenKind op = p->la.kind;
        const char *op_str = token_kind_to_str(op);
        parser_eat(p, op);
        ASTExpr *right = parse_rel_expr(p);
        node = ast_expr_binary(&p->arena, op_str, node, right);
    }
    return node;
}

static ASTExpr *parse_rel_expr(Parser *p) {
    ASTExpr *node = parse_add_expr(p);
    while (p->la.kind == TOK_LT || p->la.kind == TOK_LTE ||
           p->la.kind == TOK_GT || p->la.kind == TOK_GTE) {
        TokenKind op = p->la.kind;
        const char *op_str = token_kind_to_str(op);
        parser_eat(p, op);
        ASTExpr *right = parse_add_expr(p);
        node = ast_expr_binary(&p->arena, op_str, node, right);
    }
    return node;
}

static ASTExpr *parse_add_expr(Parser *p) {
    ASTExpr *node = parse_mult_expr(p);
    while (p->la.kind == TOK_PLUS || p->la.kind == TOK_MINUS) {
        TokenKind op = p->la.kind;
        const char *op_str = token_kind_to_str(op);
        parser_eat(p, op);
        ASTExpr *right = parse_mult_expr(p);
        node = ast_expr_binary(&p->arena, op_str, node, right);
    }
    return node;
}

static ASTExpr *parse_mult_expr(Parser *p) {
    ASTExpr *node = parse_mod_expr(p);
    while (p->la.kind == TOK_MUL || p->la.kind == TOK_DIV) {
        TokenKind op = p->la.kind;
        const char *op_str = token_kind_to_str(op);
        parser_eat(p, op);
        ASTExpr *right = parse_mod_expr(p);
        node = ast_expr_binary(&p->arena, op_str, node, right);
    }
    return node;
}

static ASTExpr *parse_mod_expr(Parser *p) {
    ASTExpr *node = parse_unary_expr(p);
    while (p->la.kind == TOK_MOD) {
        TokenKind op = p->la.kind;
        const char *op_str = token_kind_to_str(op);
        parser_eat(p, op);
        ASTExpr *right = parse_unary_expr(p);
        node = ast_expr_binary(&p->arena, op_str, node, right);
    }
    return node;
}

static ASTExpr *parse_unary_expr(Parser *p) {
    if (p->la.kind == TOK_MINUS) {
        parser_eat(p, TOK_MINUS);
        ASTExpr *inner = parse_unary_expr(p);
        return ast_expr_unary(&p->arena, EXPR_UNARY_MINUS, inner);
    }
    if (p->la.kind == TOK_NOT) {
        parser_eat(p, TOK_NOT);
        ASTExpr *inner = parse_unary_expr(p);
        return ast_expr_unary(&p->arena, EXPR_UNARY_NOT, inner);
    }
    return parse_exp_expr(p);
}

static ASTExpr *parse_exp_expr(Parser *p) {
    ASTExpr *node = parse_primary(p);
    if (p->la.kind == TOK_EXP) {
        parser_eat(p, TOK_EXP);
        ASTExpr *right = parse_unary_expr(p); /* asociatividad derecha */
        return ast_expr_binary(&p->arena, "TOK-EXP", node, right);
    }
    return node;
}

static ASTExpr *parse_primary(Parser *p) {
    TokenKind k = p->la.kind;
    if (k == TOK_NUMBER) {
        Token t = parser_comer(p, TOK_NUMBER);
        ASTExpr *e = ast_expr_number(&p->arena, t.lexeme);
        token_free(&t);
        return e;
    }
    if (k == TOK_STRING) {
        Token t = parser_comer(p, TOK_STRING);
        ASTExpr *e = ast_expr_string(&p->arena, t.lexeme);
        token_free(&t);
        return e;
    }
    if (k == TOK_TRUE) {
        parser_eat(p, TOK_TRUE);
        return ast_expr_bool(&p->arena, "true");
    }
    if (k == TOK_FALSE) {
        parser_eat(p, TOK_FALSE);
        return ast_expr_bool(&p->arena, "false");
    }
    if (k == TOK_LPAR) {
        parser_eat(p, TOK_LPAR);
        ASTExpr *e = parse_expr(p);
        parser_eat(p, TOK_RPAR);
        return ast_expr_paren(&p->arena, e);
    }
    if (k == TOK_ID) {
        char *nombre = parser_eat_id(p);
        if (p->la.kind == TOK_LPAR) {
            parser_eat(p, TOK_LPAR);
            ASTExprList args;
            ast_expr_list_init(&p->arena, &args);
            if (es_expr_start(p->la.kind)) {
                args = parse_arg_list(p);
            }
            parser_eat(p, TOK_RPAR);
            return ast_expr_call(&p->arena, nombre, args);
        }
        return ast_expr_id(&p->arena, nombre);
    }

    parser_error(p, "expresión");
    return NULL;
}
