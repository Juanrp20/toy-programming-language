#include "ast.h"
#include <stdlib.h>
#include <string.h>

void ast_expr_list_init(Arena *arena, ASTExprList *list) {
    (void)arena;
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void ast_expr_list_push(Arena *arena, ASTExprList *list, ASTExpr *item) {
    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity == 0 ? 4 : list->capacity * 2;
        ASTExpr **new_items = (ASTExpr **)arena_alloc(arena, new_cap * sizeof(ASTExpr *));
        if (list->items && list->count > 0) {
            memcpy(new_items, list->items, list->count * sizeof(ASTExpr *));
        }
        list->items = new_items;
        list->capacity = new_cap;
    }
    list->items[list->count++] = item;
}

void ast_string_list_init(Arena *arena, ASTStringList *list) {
    (void)arena;
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void ast_string_list_push(Arena *arena, ASTStringList *list, const char *item) {
    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity == 0 ? 4 : list->capacity * 2;
        char **new_items = (char **)arena_alloc(arena, new_cap * sizeof(char *));
        if (list->items && list->count > 0) {
            memcpy(new_items, list->items, list->count * sizeof(char *));
        }
        list->items = new_items;
        list->capacity = new_cap;
    }
    list->items[list->count++] = arena_strdup(arena, item);
}

void ast_stmt_list_init(Arena *arena, ASTStmtList *list) {
    (void)arena;
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void ast_stmt_list_push(Arena *arena, ASTStmtList *list, ASTStmt *item) {
    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity == 0 ? 4 : list->capacity * 2;
        ASTStmt **new_items = (ASTStmt **)arena_alloc(arena, new_cap * sizeof(ASTStmt *));
        if (list->items && list->count > 0) {
            memcpy(new_items, list->items, list->count * sizeof(ASTStmt *));
        }
        list->items = new_items;
        list->capacity = new_cap;
    }
    list->items[list->count++] = item;
}

void ast_func_list_init(Arena *arena, ASTFuncList *list) {
    (void)arena;
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void ast_func_list_push(Arena *arena, ASTFuncList *list, ASTFunc *item) {
    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity == 0 ? 4 : list->capacity * 2;
        ASTFunc **new_items = (ASTFunc **)arena_alloc(arena, new_cap * sizeof(ASTFunc *));
        if (list->items && list->count > 0) {
            memcpy(new_items, list->items, list->count * sizeof(ASTFunc *));
        }
        list->items = new_items;
        list->capacity = new_cap;
    }
    list->items[list->count++] = item;
}

ASTExpr *ast_expr_number(Arena *arena, const char *lex) {
    ASTExpr *e = (ASTExpr *)arena_alloc(arena, sizeof(ASTExpr));
    e->kind = EXPR_NUMBER;
    e->as.str_val = arena_strdup(arena, lex);
    return e;
}

ASTExpr *ast_expr_string(Arena *arena, const char *lex) {
    ASTExpr *e = (ASTExpr *)arena_alloc(arena, sizeof(ASTExpr));
    e->kind = EXPR_STRING;
    e->as.str_val = arena_strdup(arena, lex);
    return e;
}

ASTExpr *ast_expr_bool(Arena *arena, const char *val) {
    ASTExpr *e = (ASTExpr *)arena_alloc(arena, sizeof(ASTExpr));
    e->kind = EXPR_BOOL;
    e->as.bool_val = arena_strdup(arena, val);
    return e;
}

ASTExpr *ast_expr_id(Arena *arena, const char *name) {
    ASTExpr *e = (ASTExpr *)arena_alloc(arena, sizeof(ASTExpr));
    e->kind = EXPR_ID;
    e->as.str_val = arena_strdup(arena, name);
    return e;
}

ASTExpr *ast_expr_paren(Arena *arena, ASTExpr *inner) {
    ASTExpr *e = (ASTExpr *)arena_alloc(arena, sizeof(ASTExpr));
    e->kind = EXPR_PAREN;
    e->as.unary_arg = inner;
    return e;
}

ASTExpr *ast_expr_call(Arena *arena, const char *name, ASTExprList args) {
    ASTExpr *e = (ASTExpr *)arena_alloc(arena, sizeof(ASTExpr));
    e->kind = EXPR_CALL;
    e->as.call.callee = arena_strdup(arena, name);
    e->as.call.args = args;
    return e;
}

ASTExpr *ast_expr_unary(Arena *arena, ASTExprKind kind, ASTExpr *inner) {
    ASTExpr *e = (ASTExpr *)arena_alloc(arena, sizeof(ASTExpr));
    e->kind = kind;
    e->as.unary_arg = inner;
    return e;
}

ASTExpr *ast_expr_binary(Arena *arena, const char *op_str, ASTExpr *left, ASTExpr *right) {
    ASTExpr *e = (ASTExpr *)arena_alloc(arena, sizeof(ASTExpr));
    e->kind = EXPR_BINARY;
    e->as.binary.op_str = arena_strdup(arena, op_str);
    e->as.binary.left = left;
    e->as.binary.right = right;
    return e;
}

ASTStmt *ast_stmt_if(Arena *arena, ASTExpr *cond, ASTBlock *tb, ASTBlock *eb) {
    ASTStmt *s = (ASTStmt *)arena_alloc(arena, sizeof(ASTStmt));
    s->kind = STMT_IF;
    s->as.if_stmt.cond = cond;
    s->as.if_stmt.then_block = tb;
    s->as.if_stmt.else_block = eb;
    return s;
}

ASTStmt *ast_stmt_while(Arena *arena, ASTExpr *cond, ASTBlock *body) {
    ASTStmt *s = (ASTStmt *)arena_alloc(arena, sizeof(ASTStmt));
    s->kind = STMT_WHILE;
    s->as.while_stmt.cond = cond;
    s->as.while_stmt.body = body;
    return s;
}

ASTStmt *ast_stmt_for(Arena *arena, const char *var, ASTExpr *start, ASTExpr *end, ASTExpr *step, ASTBlock *body) {
    ASTStmt *s = (ASTStmt *)arena_alloc(arena, sizeof(ASTStmt));
    s->kind = STMT_FOR;
    s->as.for_stmt.var = arena_strdup(arena, var);
    s->as.for_stmt.start = start;
    s->as.for_stmt.end = end;
    s->as.for_stmt.step = step;
    s->as.for_stmt.body = body;
    return s;
}

ASTStmt *ast_stmt_print(Arena *arena, int is_println, ASTExpr *expr) {
    ASTStmt *s = (ASTStmt *)arena_alloc(arena, sizeof(ASTStmt));
    s->kind = is_println ? STMT_PRINTLN : STMT_PRINT;
    s->as.print_expr = expr;
    return s;
}

ASTStmt *ast_stmt_local(Arena *arena, const char *var, ASTExpr *expr) {
    ASTStmt *s = (ASTStmt *)arena_alloc(arena, sizeof(ASTStmt));
    s->kind = STMT_LOCAL;
    s->as.local_stmt.var = arena_strdup(arena, var);
    s->as.local_stmt.expr = expr;
    return s;
}

ASTStmt *ast_stmt_assign(Arena *arena, const char *var, ASTExpr *expr) {
    ASTStmt *s = (ASTStmt *)arena_alloc(arena, sizeof(ASTStmt));
    s->kind = STMT_ASSIGN;
    s->as.assign_stmt.var = arena_strdup(arena, var);
    s->as.assign_stmt.expr = expr;
    return s;
}

ASTStmt *ast_stmt_call(Arena *arena, const char *name, ASTExprList args) {
    ASTStmt *s = (ASTStmt *)arena_alloc(arena, sizeof(ASTStmt));
    s->kind = STMT_CALL;
    s->as.call_stmt.name = arena_strdup(arena, name);
    s->as.call_stmt.args = args;
    return s;
}

ASTStmt *ast_stmt_ret(Arena *arena, ASTExpr *expr) {
    ASTStmt *s = (ASTStmt *)arena_alloc(arena, sizeof(ASTStmt));
    s->kind = STMT_RET;
    s->as.ret_expr = expr;
    return s;
}

ASTBlock *ast_block_create(Arena *arena, ASTStmtList stmts, ASTStmt *ret_stmt) {
    ASTBlock *b = (ASTBlock *)arena_alloc(arena, sizeof(ASTBlock));
    b->stmts = stmts;
    b->ret_stmt = ret_stmt;
    return b;
}

ASTFunc *ast_func_create(Arena *arena, const char *name, ASTStringList params, ASTBlock *body) {
    ASTFunc *f = (ASTFunc *)arena_alloc(arena, sizeof(ASTFunc));
    f->is_main = 0;
    f->name = arena_strdup(arena, name);
    f->params = params;
    f->body = body;
    return f;
}

ASTFunc *ast_main_create(Arena *arena, ASTBlock *body) {
    ASTFunc *f = (ASTFunc *)arena_alloc(arena, sizeof(ASTFunc));
    f->is_main = 1;
    f->name = NULL;
    f->params.items = NULL;
    f->params.count = 0;
    f->params.capacity = 0;
    f->body = body;
    return f;
}

ASTProgram *ast_program_create(Arena *arena, ASTFuncList funcs) {
    ASTProgram *p = (ASTProgram *)arena_alloc(arena, sizeof(ASTProgram));
    p->funcs = funcs;
    return p;
}

static void print_expr(const ASTExpr *e, FILE *out);
static void print_block(const ASTBlock *b, FILE *out);

static void print_expr(const ASTExpr *e, FILE *out) {
    if (!e) {
        fprintf(out, "None");
        return;
    }
    switch (e->kind) {
        case EXPR_NUMBER:
            fprintf(out, "('number', '%s')", e->as.str_val);
            break;
        case EXPR_STRING:
            fprintf(out, "('string', '%s')", e->as.str_val);
            break;
        case EXPR_BOOL:
            fprintf(out, "('bool', '%s')", e->as.bool_val);
            break;
        case EXPR_ID:
            fprintf(out, "('id', '%s')", e->as.str_val);
            break;
        case EXPR_PAREN:
            fprintf(out, "('paren', ");
            print_expr(e->as.unary_arg, out);
            fprintf(out, ")");
            break;
        case EXPR_UNARY_MINUS:
            fprintf(out, "('unary_minus', ");
            print_expr(e->as.unary_arg, out);
            fprintf(out, ")");
            break;
        case EXPR_UNARY_NOT:
            fprintf(out, "('unary_not', ");
            print_expr(e->as.unary_arg, out);
            fprintf(out, ")");
            break;
        case EXPR_BINARY:
            fprintf(out, "('%s', ", e->as.binary.op_str);
            print_expr(e->as.binary.left, out);
            fprintf(out, ", ");
            print_expr(e->as.binary.right, out);
            fprintf(out, ")");
            break;
        case EXPR_CALL:
            fprintf(out, "('call', '%s', [", e->as.call.callee);
            for (size_t i = 0; i < e->as.call.args.count; i++) {
                if (i > 0) fprintf(out, ", ");
                print_expr(e->as.call.args.items[i], out);
            }
            fprintf(out, "])");
            break;
    }
}

static void print_stmt(const ASTStmt *s, FILE *out) {
    if (!s) return;
    switch (s->kind) {
        case STMT_IF:
            fprintf(out, "('if', ");
            print_expr(s->as.if_stmt.cond, out);
            fprintf(out, ", ");
            print_block(s->as.if_stmt.then_block, out);
            fprintf(out, ", ");
            if (s->as.if_stmt.else_block) {
                print_block(s->as.if_stmt.else_block, out);
            } else {
                fprintf(out, "None");
            }
            fprintf(out, ")");
            break;
        case STMT_WHILE:
            fprintf(out, "('while', ");
            print_expr(s->as.while_stmt.cond, out);
            fprintf(out, ", ");
            print_block(s->as.while_stmt.body, out);
            fprintf(out, ")");
            break;
        case STMT_FOR:
            fprintf(out, "('for', '%s', ", s->as.for_stmt.var);
            print_expr(s->as.for_stmt.start, out);
            fprintf(out, ", ");
            print_expr(s->as.for_stmt.end, out);
            fprintf(out, ", ");
            if (s->as.for_stmt.step) {
                print_expr(s->as.for_stmt.step, out);
            } else {
                fprintf(out, "None");
            }
            fprintf(out, ", ");
            print_block(s->as.for_stmt.body, out);
            fprintf(out, ")");
            break;
        case STMT_PRINT:
            fprintf(out, "('print', ");
            print_expr(s->as.print_expr, out);
            fprintf(out, ")");
            break;
        case STMT_PRINTLN:
            fprintf(out, "('println', ");
            print_expr(s->as.print_expr, out);
            fprintf(out, ")");
            break;
        case STMT_LOCAL:
            fprintf(out, "('local', '%s', ", s->as.local_stmt.var);
            print_expr(s->as.local_stmt.expr, out);
            fprintf(out, ")");
            break;
        case STMT_ASSIGN:
            fprintf(out, "('assign', '%s', ", s->as.assign_stmt.var);
            print_expr(s->as.assign_stmt.expr, out);
            fprintf(out, ")");
            break;
        case STMT_CALL:
            fprintf(out, "('call_stmt', '%s', [", s->as.call_stmt.name);
            for (size_t i = 0; i < s->as.call_stmt.args.count; i++) {
                if (i > 0) fprintf(out, ", ");
                print_expr(s->as.call_stmt.args.items[i], out);
            }
            fprintf(out, "])");
            break;
        case STMT_RET:
            fprintf(out, "('ret', ");
            if (s->as.ret_expr) {
                print_expr(s->as.ret_expr, out);
            } else {
                fprintf(out, "None");
            }
            fprintf(out, ")");
            break;
    }
}

static void print_block(const ASTBlock *b, FILE *out) {
    fprintf(out, "('block', [");
    for (size_t i = 0; i < b->stmts.count; i++) {
        if (i > 0) fprintf(out, ", ");
        print_stmt(b->stmts.items[i], out);
    }
    fprintf(out, "], ");
    if (b->ret_stmt) {
        print_stmt(b->ret_stmt, out);
    } else {
        fprintf(out, "None");
    }
    fprintf(out, ")");
}

void ast_print_program(const ASTProgram *prog, FILE *out) {
    fprintf(out, "('program', [");
    for (size_t i = 0; i < prog->funcs.count; i++) {
        if (i > 0) fprintf(out, ", ");
        ASTFunc *f = prog->funcs.items[i];
        if (f->is_main) {
            fprintf(out, "('main', ");
            print_block(f->body, out);
            fprintf(out, ")");
        } else {
            fprintf(out, "('func', '%s', [", f->name);
            for (size_t j = 0; j < f->params.count; j++) {
                if (j > 0) fprintf(out, ", ");
                fprintf(out, "'%s'", f->params.items[j]);
            }
            fprintf(out, "], ");
            print_block(f->body, out);
            fprintf(out, ")");
        }
    }
    fprintf(out, "])\n");
}
