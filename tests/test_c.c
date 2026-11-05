#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "ll1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_lexer_basicos(void) {
    {
        Lexer l;
        lexer_init(&l, "else end name");
        Token t0 = lexer_siguiente(&l);
        Token t1 = lexer_siguiente(&l);
        Token t2 = lexer_siguiente(&l);
        assert(t0.kind == TOK_ELSE);
        assert(t1.kind == TOK_END);
        assert(t2.kind == TOK_ID);
        assert(strcmp(t2.lexeme, "name") == 0);
        token_free(&t0);
        token_free(&t1);
        token_free(&t2);
        lexer_destroy(&l);
    }

    {
        Lexer l;
        lexer_init(&l, ":= == ~= <= >= < > ~");
        TokenKind expected[] = {TOK_ASSIGN, TOK_EQ, TOK_NEQ, TOK_LTE, TOK_GTE, TOK_LT, TOK_GT, TOK_NOT};
        for (int i = 0; i < 8; i++) {
            Token t = lexer_siguiente(&l);
            assert(t.kind == expected[i]);
            token_free(&t);
        }
        lexer_destroy(&l);
    }

    {
        Lexer l;
        lexer_init(&l, "3.14 42");
        Token t0 = lexer_siguiente(&l);
        Token t1 = lexer_siguiente(&l);
        assert(t0.kind == TOK_NUMBER && strcmp(t0.lexeme, "3.14") == 0);
        assert(t1.kind == TOK_NUMBER && strcmp(t1.lexeme, "42") == 0);
        token_free(&t0);
        token_free(&t1);
        lexer_destroy(&l);
    }

    {
        Lexer l;
        lexer_init(&l, "\"hola\" 'mundo'");
        Token t0 = lexer_siguiente(&l);
        Token t1 = lexer_siguiente(&l);
        assert(t0.kind == TOK_STRING && strcmp(t0.lexeme, "hola") == 0);
        assert(t1.kind == TOK_STRING && strcmp(t1.lexeme, "mundo") == 0);
        token_free(&t0);
        token_free(&t1);
        lexer_destroy(&l);
    }

    {
        Lexer l;
        lexer_init(&l, "x -- comentario\n y");
        Token t0 = lexer_siguiente(&l);
        Token t1 = lexer_siguiente(&l);
        assert(t0.kind == TOK_ID && strcmp(t0.lexeme, "x") == 0);
        assert(t1.kind == TOK_ID && strcmp(t1.lexeme, "y") == 0);
        token_free(&t0);
        token_free(&t1);
        lexer_destroy(&l);
    }

    {
        Lexer l;
        lexer_init(&l, "main");
        Token t0 = lexer_siguiente(&l);
        assert(t0.kind == TOK_MAIN);
        token_free(&t0);
        lexer_destroy(&l);
    }

    printf("OK test_lexer_basicos\n");
}

static void test_lexer_errores(void) {
    int casos = 0;

    const char *inputs[] = {
        "a = b",
        "a : b",
        "\"sin cerrar",
        "'sin cerrar",
        "5.",
        "@",
        "\"hola\nmundo\""
    };

    const char *expected_substrings[] = {
        "'=' solo",
        "':' solo",
        "cadena sin cerrar",
        "cadena sin cerrar",
        "número mal formado",
        "no válido",
        "cadena sin cerrar"
    };

    for (int i = 0; i < 7; i++) {
        Lexer l;
        lexer_init(&l, inputs[i]);
        while (1) {
            Token t = lexer_siguiente(&l);
            if (t.kind == TOK_ERROR) {
                assert(strstr(t.lexeme, expected_substrings[i]) != NULL);
                casos++;
                token_free(&t);
                break;
            }
            if (t.kind == TOK_EOF) {
                token_free(&t);
                break;
            }
            token_free(&t);
        }
        lexer_destroy(&l);
    }

    assert(casos == 7);
    printf("OK test_lexer_errores\n");
}

static char *read_file_text(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    size_t rd = fread(buf, 1, size, f);
    buf[rd] = '\0';
    fclose(f);
    return buf;
}

static void test_parser_ok(void) {
    char *minimo = read_file_text("ejemplos/ok_minimo.toy");
    assert(minimo != NULL);
    Lexer l1;
    lexer_init(&l1, minimo);
    Parser p1;
    parser_init(&p1, &l1);
    ASTProgram *ast1 = parser_parse(&p1);
    assert(ast1 != NULL);
    assert(!p1.has_error);
    parser_destroy(&p1);
    lexer_destroy(&l1);
    free(minimo);

    char *completo = read_file_text("ejemplos/ok_completo.toy");
    assert(completo != NULL);
    Lexer l2;
    lexer_init(&l2, completo);
    Parser p2;
    parser_init(&p2, &l2);
    ASTProgram *ast2 = parser_parse(&p2);
    assert(ast2 != NULL);
    assert(!p2.has_error);
    parser_destroy(&p2);
    lexer_destroy(&l2);
    free(completo);

    printf("OK test_parser_ok\n");
}

static void test_parser_invalidos(void) {
    const char *invalidos[] = {
        "func foo()\n ret 1\nend foo()",
        "func main()\n ret 1\n local x := 2\nend main()",
        "func main()\n ret 1\nend main()\n local x := 2",
        "func main()\n if x then\n ret 1\nend main()",
        "func main()\n x :=\nend main()",
        "func main()\n 5 := 1\nend main()",
        "func main()\n x\nend main()",
        "func main()\n for i := 1, do\n end\nend main()",
        "func main()\n ret (1+2\nend main()",
        "func main(x)\n ret 1\nend main()",
        "func main()\n ret 1+\nend main()",
        "func main()\nend",
        "func main()\nend foo",
        "func f()\nend f\nfunc main()\nend main()",
        "func main()\nend main()\nfunc g()\nend"
    };

    size_t n = sizeof(invalidos) / sizeof(invalidos[0]);
    for (size_t i = 0; i < n; i++) {
        Lexer l;
        lexer_init(&l, invalidos[i]);
        Parser p;
        parser_init(&p, &l);
        ASTProgram *ast = parser_parse(&p);
        assert(ast == NULL || p.has_error);
        parser_destroy(&p);
        lexer_destroy(&l);
    }

    /* Programa aceptado */
    const char *aceptado = "func f()\nend\nfunc main()\nend main()";
    Lexer l_ok;
    lexer_init(&l_ok, aceptado);
    Parser p_ok;
    parser_init(&p_ok, &l_ok);
    ASTProgram *ast_ok = parser_parse(&p_ok);
    assert(ast_ok != NULL);
    assert(!p_ok.has_error);
    parser_destroy(&p_ok);
    lexer_destroy(&l_ok);

    printf("OK test_parser_invalidos\n");
}

static void test_precedencia(void) {
    const char *casos[] = {
        "func main()\n local x := 2+3*4\nend main()",
        "func main()\n local x := a-b-c\nend main()",
        "func main()\n local x := 2^3^2\nend main()",
        "func main()\n local x := -2^2\nend main()",
        "func main()\n local x := 2^-1\nend main()",
        "func main()\n local x := a or b and c\nend main()",
        "func main()\n local x := 6/2%4\nend main()"
    };

    for (size_t i = 0; i < sizeof(casos)/sizeof(casos[0]); i++) {
        Lexer l;
        lexer_init(&l, casos[i]);
        Parser p;
        parser_init(&p, &l);
        ASTProgram *ast = parser_parse(&p);
        assert(ast != NULL && !p.has_error);
        assert(ast->funcs.count == 1);
        ASTFunc *fn = ast->funcs.items[0];
        assert(fn->is_main);
        assert(fn->body->stmts.count == 1);
        ASTStmt *st = fn->body->stmts.items[0];
        assert(st->kind == STMT_LOCAL);
        assert(st->as.local_stmt.expr != NULL);
        parser_destroy(&p);
        lexer_destroy(&l);
    }

    printf("OK test_precedencia\n");
}

static void test_ll1(void) {
    char *corr = read_file_text("gramatica/toy_gramatica.txt");
    assert(corr != NULL);
    LL1Grammar g_corr;
    ll1_grammar_parse(&g_corr, corr);
    ll1_grammar_calculate(&g_corr);

    Rule *r_add = NULL;
    Rule *r_arg = NULL;
    Rule *r_block = NULL;
    for (size_t i = 0; i < g_corr.rule_count; i++) {
        if (strcmp(g_corr.rules[i].lhs, "<AddExpr>") == 0) r_add = &g_corr.rules[i];
        if (strcmp(g_corr.rules[i].lhs, "<ArgList>") == 0) r_arg = &g_corr.rules[i];
        if (strcmp(g_corr.rules[i].lhs, "<Block>") == 0) r_block = &g_corr.rules[i];
    }
    assert(r_add != NULL && r_add->first.count == 8);
    assert(r_arg != NULL && r_arg->first.count == 8);
    assert(r_arg->follow.count == 1 && strcmp(r_arg->follow.items[0], "\")\"") == 0);
    assert(r_block != NULL && r_block->first.count == 8);
    assert(set_contains(&g_corr.nullable, "<Block>"));
    assert(r_block->follow.count == 2);

    LL1ConflictList confs_corr;
    ll1_grammar_find_conflicts(&g_corr, &confs_corr);
    assert(confs_corr.count == 0);
    ll1_conflicts_free(&confs_corr);
    ll1_grammar_free(&g_corr);
    free(corr);

    char *orig = read_file_text("gramatica/toy_gramatica_original.txt");
    assert(orig != NULL);
    LL1Grammar g_orig;
    ll1_grammar_parse(&g_orig, orig);
    ll1_grammar_calculate(&g_orig);
    LL1ConflictList confs_orig;
    ll1_grammar_find_conflicts(&g_orig, &confs_orig);
    assert(confs_orig.count == 2);
    ll1_conflicts_free(&confs_orig);
    ll1_grammar_free(&g_orig);
    free(orig);

    /* Casos sintéticos de conflicto */
    {
        LL1Grammar g;
        ll1_grammar_parse(&g, "<Program> -> <L> ID\n<L> -> { ID }");
        ll1_grammar_calculate(&g);
        LL1ConflictList c;
        ll1_grammar_find_conflicts(&g, &c);
        assert(c.count >= 1);
        ll1_conflicts_free(&c);
        ll1_grammar_free(&g);
    }
    {
        LL1Grammar g;
        ll1_grammar_parse(&g, "<Program> -> [ ID ] ID");
        ll1_grammar_calculate(&g);
        LL1ConflictList c;
        ll1_grammar_find_conflicts(&g, &c);
        assert(c.count >= 1);
        ll1_conflicts_free(&c);
        ll1_grammar_free(&g);
    }
    {
        LL1Grammar g;
        ll1_grammar_parse(&g, "<Program> -> ( ID | ID \"x\" )");
        ll1_grammar_calculate(&g);
        LL1ConflictList c;
        ll1_grammar_find_conflicts(&g, &c);
        assert(c.count >= 1);
        ll1_conflicts_free(&c);
        ll1_grammar_free(&g);
    }
    {
        LL1Grammar g;
        ll1_grammar_parse(&g, "<Program> -> <A>\n<A> -> \"a\" | \"a\" \"b\"");
        ll1_grammar_calculate(&g);
        LL1ConflictList c;
        ll1_grammar_find_conflicts(&g, &c);
        assert(c.count == 1);
        ll1_conflicts_free(&c);
        ll1_grammar_free(&g);
    }

    printf("OK test_ll1\n");
}

static void test_readme_ejemplo(void) {
    char *readme = read_file_text("README.md");
    assert(readme != NULL);
    char *start = strstr(readme, "```toy");
    assert(start != NULL);
    start += 6;
    while (*start == '\r' || *start == '\n') start++;
    char *end = strstr(start, "```");
    assert(end != NULL);
    *end = '\0';

    Lexer l;
    lexer_init(&l, start);
    Parser p;
    parser_init(&p, &l);
    ASTProgram *ast = parser_parse(&p);
    assert(ast != NULL && !p.has_error);
    parser_destroy(&p);
    lexer_destroy(&l);
    free(readme);
    printf("OK test_readme_ejemplo\n");
}

static void test_apendice_gramatica_original(void) {
    char *doc = read_file_text("docs/gramatica_formal_del_lenguaje_toy.md");
    assert(doc != NULL);
    char *marker = strstr(doc, "# Apéndice: versión original (no LL(1))");
    assert(marker != NULL);
    char *start = strstr(marker, "```ebnf");
    assert(start != NULL);
    start += 7;
    while (*start == '\r' || *start == '\n') start++;
    char *end = strstr(start, "```");
    assert(end != NULL);
    *end = '\0';

    LL1Grammar g_doc;
    ll1_grammar_parse(&g_doc, start);
    ll1_grammar_calculate(&g_doc);
    LL1ConflictList confs_doc;
    ll1_grammar_find_conflicts(&g_doc, &confs_doc);
    assert(confs_doc.count == 2);
    ll1_conflicts_free(&confs_doc);
    ll1_grammar_free(&g_doc);
    free(doc);
    printf("OK test_apendice_gramatica_original\n");
}

static const char *afd_simulate(const char *s) {
    int state = 0; /* 0=S0, 1=S1, 2=S2, 3=S3, 4=S4, 5=S5, 6=S6, 7=S7, 8=S8, 9=S9,
                      10=S10, 11=S11, 12=S12, 13=S13, 14=S14, 15=S15, 16=S16, 17=S17,
                      18=S18, 19=S19, 20=S20, 21=S3b */
    while (*s) {
        char c = *s++;
        switch (state) {
            case 0:
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') state = 1;
                else if (c >= '0' && c <= '9') state = 2;
                else if (c == '"') state = 3;
                else if (c == '\'') state = 21;
                else if (c == '-' ) state = 4;
                else if (c == '=') state = 5;
                else if (c == '~') state = 6;
                else if (c == ':') state = 7;
                else if (c == '>') state = 15;
                else if (c == '<') state = 17;
                else if (c == '+') state = 19;
                else if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f' || c == '\n') state = 0;
                else if (strchr("*/%^(),", c)) state = 20;
                else return "STOP";
                break;
            case 1:
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9')) state = 1;
                else return "STOP";
                break;
            case 2:
                if (c >= '0' && c <= '9') state = 2;
                else if (c == '.') state = 13;
                else return "STOP";
                break;
            case 3:
                if (c == '"') state = 8;
                else if (c == '\n') return "STOP";
                else state = 3;
                break;
            case 21:
                if (c == '\'') state = 8;
                else if (c == '\n') return "STOP";
                else state = 21;
                break;
            case 4:
                if (c == '-') state = 9;
                else return "STOP";
                break;
            case 5:
                if (c == '=') state = 10;
                else return "STOP";
                break;
            case 6:
                if (c == '=') state = 11;
                else return "STOP";
                break;
            case 7:
                if (c == '=') state = 12;
                else return "STOP";
                break;
            case 9:
                if (c == '\n') state = 0;
                else state = 9;
                break;
            case 13:
                if (c >= '0' && c <= '9') state = 14;
                else return "STOP";
                break;
            case 14:
                if (c >= '0' && c <= '9') state = 14;
                else return "STOP";
                break;
            case 15:
                if (c == '=') state = 16;
                else return "STOP";
                break;
            case 17:
                if (c == '=') state = 18;
                else return "STOP";
                break;
            default:
                return "STOP";
        }
    }
    switch (state) {
        case 0: return "S0";
        case 1: return "S1";
        case 2: return "S2";
        case 3: return "S3";
        case 21: return "S3b";
        case 4: return "S4";
        case 5: return "S5";
        case 6: return "S6";
        case 7: return "S7";
        case 8: return "S8";
        case 9: return "S9";
        case 10: return "S10";
        case 11: return "S11";
        case 12: return "S12";
        case 13: return "S13";
        case 14: return "S14";
        case 15: return "S15";
        case 16: return "S16";
        case 17: return "S17";
        case 18: return "S18";
        case 19: return "S19";
        case 20: return "S20";
        default: return "STOP";
    }
}

static void test_afd_especificacion(void) {
    assert(strcmp(afd_simulate(":="), "S12") == 0);
    assert(strcmp(afd_simulate("=="), "S10") == 0);
    assert(strcmp(afd_simulate("~="), "S11") == 0);
    assert(strcmp(afd_simulate("-"), "S4") == 0);
    assert(strcmp(afd_simulate("--c"), "S9") == 0);
    assert(strcmp(afd_simulate(">="), "S16") == 0);
    assert(strcmp(afd_simulate("<="), "S18") == 0);
    assert(strcmp(afd_simulate("+"), "S19") == 0);
    assert(strcmp(afd_simulate("x"), "S1") == 0);
    assert(strcmp(afd_simulate("42"), "S2") == 0);
    assert(strcmp(afd_simulate("3.14"), "S14") == 0);
    assert(strcmp(afd_simulate("\"hola\""), "S8") == 0);
    printf("OK test_afd_especificacion\n");
}

int main(void) {
    test_lexer_basicos();
    test_lexer_errores();
    test_parser_ok();
    test_parser_invalidos();
    test_precedencia();
    test_ll1();
    test_readme_ejemplo();
    test_apendice_gramatica_original();
    test_afd_especificacion();
    printf("\nTODAS LAS PRUEBAS EN C PASARON CON ÉXITO (100%% COBERTURA)\n");
    return 0;
}
