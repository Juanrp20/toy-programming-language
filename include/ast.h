#ifndef TOY_AST_H
#define TOY_AST_H

#include "token.h"
#include "arena.h"
#include <stdio.h>

typedef struct ASTExpr ASTExpr;
typedef struct ASTStmt ASTStmt;
typedef struct ASTBlock ASTBlock;
typedef struct ASTFunc ASTFunc;
typedef struct ASTProgram ASTProgram;

typedef struct {
    ASTExpr **items;
    size_t count;
    size_t capacity;
} ASTExprList;

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} ASTStringList;

typedef struct {
    ASTStmt **items;
    size_t count;
    size_t capacity;
} ASTStmtList;

typedef struct {
    ASTFunc **items;
    size_t count;
    size_t capacity;
} ASTFuncList;

typedef enum {
    EXPR_NUMBER,
    EXPR_STRING,
    EXPR_BOOL,
    EXPR_ID,
    EXPR_PAREN,
    EXPR_CALL,
    EXPR_UNARY_MINUS,
    EXPR_UNARY_NOT,
    EXPR_BINARY
} ASTExprKind;

struct ASTExpr {
    ASTExprKind kind;
    union {
        char *str_val;       /* EXPR_NUMBER, EXPR_STRING, EXPR_ID */
        const char *bool_val;/* "true" o "false" */
        ASTExpr *unary_arg;  /* EXPR_UNARY_MINUS, EXPR_UNARY_NOT, EXPR_PAREN */
        struct {
            char *callee;
            ASTExprList args;
        } call;
        struct {
            const char *op_str;
            ASTExpr *left;
            ASTExpr *right;
        } binary;
    } as;
};

typedef enum {
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_PRINT,
    STMT_PRINTLN,
    STMT_LOCAL,
    STMT_ASSIGN,
    STMT_CALL,
    STMT_RET
} ASTStmtKind;

struct ASTStmt {
    ASTStmtKind kind;
    union {
        struct {
            ASTExpr *cond;
            ASTBlock *then_block;
            ASTBlock *else_block; /* NULL si no hay else */
        } if_stmt;
        struct {
            ASTExpr *cond;
            ASTBlock *body;
        } while_stmt;
        struct {
            char *var;
            ASTExpr *start;
            ASTExpr *end;
            ASTExpr *step;       /* NULL si no hay paso */
            ASTBlock *body;
        } for_stmt;
        ASTExpr *print_expr;     /* STMT_PRINT, STMT_PRINTLN */
        struct {
            char *var;
            ASTExpr *expr;
        } local_stmt;
        struct {
            char *var;
            ASTExpr *expr;
        } assign_stmt;
        struct {
            char *name;
            ASTExprList args;
        } call_stmt;
        ASTExpr *ret_expr;       /* NULL si 'ret' sin expr */
    } as;
};

struct ASTBlock {
    ASTStmtList stmts;
    ASTStmt *ret_stmt;           /* NULL si no hay return */
};

struct ASTFunc {
    int is_main;                 /* 1 para main, 0 para func normal */
    char *name;                  /* NULL si is_main == 1 */
    ASTStringList params;
    ASTBlock *body;
};

struct ASTProgram {
    ASTFuncList funcs;
};

/* Constructores (utilizan Arena para gestión automática) */
ASTExpr *ast_expr_number(Arena *arena, const char *lex);
ASTExpr *ast_expr_string(Arena *arena, const char *lex);
ASTExpr *ast_expr_bool(Arena *arena, const char *val);
ASTExpr *ast_expr_id(Arena *arena, const char *name);
ASTExpr *ast_expr_paren(Arena *arena, ASTExpr *inner);
ASTExpr *ast_expr_call(Arena *arena, const char *name, ASTExprList args);
ASTExpr *ast_expr_unary(Arena *arena, ASTExprKind kind, ASTExpr *inner);
ASTExpr *ast_expr_binary(Arena *arena, const char *op_str, ASTExpr *left, ASTExpr *right);

ASTStmt *ast_stmt_if(Arena *arena, ASTExpr *cond, ASTBlock *tb, ASTBlock *eb);
ASTStmt *ast_stmt_while(Arena *arena, ASTExpr *cond, ASTBlock *body);
ASTStmt *ast_stmt_for(Arena *arena, const char *var, ASTExpr *start, ASTExpr *end, ASTExpr *step, ASTBlock *body);
ASTStmt *ast_stmt_print(Arena *arena, int is_println, ASTExpr *expr);
ASTStmt *ast_stmt_local(Arena *arena, const char *var, ASTExpr *expr);
ASTStmt *ast_stmt_assign(Arena *arena, const char *var, ASTExpr *expr);
ASTStmt *ast_stmt_call(Arena *arena, const char *name, ASTExprList args);
ASTStmt *ast_stmt_ret(Arena *arena, ASTExpr *expr);

ASTBlock *ast_block_create(Arena *arena, ASTStmtList stmts, ASTStmt *ret_stmt);
ASTFunc *ast_func_create(Arena *arena, const char *name, ASTStringList params, ASTBlock *body);
ASTFunc *ast_main_create(Arena *arena, ASTBlock *body);
ASTProgram *ast_program_create(Arena *arena, ASTFuncList funcs);

void ast_expr_list_init(Arena *arena, ASTExprList *list);
void ast_expr_list_push(Arena *arena, ASTExprList *list, ASTExpr *item);

void ast_string_list_init(Arena *arena, ASTStringList *list);
void ast_string_list_push(Arena *arena, ASTStringList *list, const char *item);

void ast_stmt_list_init(Arena *arena, ASTStmtList *list);
void ast_stmt_list_push(Arena *arena, ASTStmtList *list, ASTStmt *item);

void ast_func_list_init(Arena *arena, ASTFuncList *list);
void ast_func_list_push(Arena *arena, ASTFuncList *list, ASTFunc *item);

void ast_print_program(const ASTProgram *prog, FILE *out);

#endif /* TOY_AST_H */
