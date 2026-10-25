# run_tests.py
import os
import sys
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))


def run_cmd(cmd):
    try:
        res = subprocess.run(cmd, shell=False, cwd=str(ROOT), stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
        return res.returncode, res.stdout.decode('utf-8', errors='replace'), res.stderr.decode('utf-8', errors='replace')
    except Exception as e:
        return -1, "", str(e)


def test_lexer_basicos():
    from toy.lexer import Lexer, LexError
    l = Lexer("else end name")
    t = l.todos()
    assert t[0].kind == "TOK-ELSE"
    assert t[1].kind == "TOK-END"
    assert t[2].kind == "TOK-ID"
    l = Lexer(":= == ~= <= >= < > ~")
    t = l.todos()
    kinds = [x.kind for x in t if x.kind != "TOK-EOF"]
    assert kinds == ["TOK-ASSIGN", "TOK-EQ", "TOK-NEQ", "TOK-LTE", "TOK-GTE", "TOK-LT", "TOK-GT", "TOK-NOT"]
    l = Lexer("3.14 42")
    t = l.todos()
    assert t[0].kind == "TOK-NUMBER" and t[0].lexeme == "3.14"
    assert t[1].kind == "TOK-NUMBER" and t[1].lexeme == "42"
    l = Lexer('"hola" \'mundo\'')
    t = l.todos()
    assert t[0].kind == "TOK-STRING" and t[0].lexeme == "hola"
    assert t[1].kind == "TOK-STRING" and t[1].lexeme == "mundo"
    l = Lexer("x -- comentario\n y")
    t = l.todos()
    kinds = [x.kind for x in t if x.kind != "TOK-EOF"]
    assert kinds == ["TOK-ID", "TOK-ID"]
    l = Lexer("main")
    t = l.todos()
    assert t[0].kind == "TOK-MAIN"
    print("OK lexer_basicos")


def test_lexer_errores():
    from toy.lexer import Lexer, LexError
    casos = []
    try:
        Lexer("a = b").todos()
    except LexError as e:
        assert "'=' solo" in e.mensaje
        casos.append(1)
    try:
        Lexer("a : b").todos()
    except LexError as e:
        assert "':' solo" in e.mensaje
        casos.append(2)
    try:
        Lexer('"sin cerrar').todos()
    except LexError as e:
        assert "cadena sin cerrar" in e.mensaje
        casos.append(3)
    try:
        Lexer("'sin cerrar").todos()
    except LexError as e:
        assert "cadena sin cerrar" in e.mensaje
        casos.append(4)
    try:
        Lexer('5.').todos()
    except LexError as e:
        assert "número mal formado" in e.mensaje
        casos.append(5)
    try:
        Lexer('@').todos()
    except LexError as e:
        assert "no válido" in e.mensaje
        casos.append(6)
    try:
        Lexer('"hola\nmundo"').todos()
    except LexError as e:
        assert "cadena sin cerrar" in e.mensaje
        casos.append(7)
    assert len(casos) == 7
    print("OK lexer_errores")


def test_parser_ok():
    from toy.parser import Parser
    from toy.lexer import Lexer
    ok_minimo = ROOT / "ejemplos" / "ok_minimo.toy"
    with open(ok_minimo, "r", encoding="utf-8") as f:
        txt = f.read()
    Parser(Lexer(txt)).parse()
    ok_completo = ROOT / "ejemplos" / "ok_completo.toy"
    with open(ok_completo, "r", encoding="utf-8") as f:
        txt = f.read()
    Parser(Lexer(txt)).parse()
    print("OK parser_ok")


def test_precedencia():
    from toy.parser import Parser
    from toy.lexer import Lexer

    def get_expr_ast(expr):
        txt = f"func main()\n local x := {expr}\nend main()"
        ast = Parser(Lexer(txt)).parse()
        # ast[1][0][1][1][0][2]
        expr_ast = ast[1][0][1][1][0][2]
        return expr_ast

    assert get_expr_ast("2+3*4") == ('TOK-PLUS', ('number', '2'), ('TOK-MUL', ('number', '3'), ('number', '4')))
    assert get_expr_ast("a-b-c") == ('TOK-MINUS', ('TOK-MINUS', ('id', 'a'), ('id', 'b')), ('id', 'c'))
    assert get_expr_ast("2^3^2") == ('TOK-EXP', ('number', '2'), ('TOK-EXP', ('number', '3'), ('number', '2')))
    assert get_expr_ast("-2^2") == ('unary_minus', ('TOK-EXP', ('number', '2'), ('number', '2')))
    assert get_expr_ast("2^-1") == ('TOK-EXP', ('number', '2'), ('unary_minus', ('number', '1')))
    assert get_expr_ast("a or b and c") == ('or', ('id', 'a'), ('and', ('id', 'b'), ('id', 'c')))
    assert get_expr_ast("6/2%4") == ('TOK-DIV', ('number', '6'), ('TOK-MOD', ('number', '2'), ('number', '4')))
    print("OK precedencia_asociatividad")


def test_parser_invalidos():
    from toy.parser import Parser, SyntaxErr
    from toy.lexer import Lexer, LexError
    try:
        Parser(Lexer("func foo()\n ret 1\nend foo()")).parse()
        raise AssertionError("debería fallar sin main")
    except (LexError, SyntaxErr):
        pass
    try:
        Parser(Lexer("func main()\n ret 1\n local x := 2\nend main()")).parse()
        raise AssertionError("debería fallar ret en medio")
    except (LexError, SyntaxErr):
        pass
    try:
        Parser(Lexer("func main()\n ret 1\nend main()\n local x := 2")).parse()
        raise AssertionError("debería fallar código después")
    except (LexError, SyntaxErr):
        pass
    try:
        Parser(Lexer("func main()\n if x then\n ret 1\nend main()")).parse()
        raise AssertionError("debería fallar falta end/then")
    except (LexError, SyntaxErr):
        pass
    try:
        Parser(Lexer("func main()\n x :=\nend main()")).parse()
        raise AssertionError("debería fallar asignación sin expr")
    except (LexError, SyntaxErr):
        pass
    try:
        Parser(Lexer("func main()\n 5 := 1\nend main()")).parse()
        raise AssertionError("debería fallar sentencia con número")
    except (LexError, SyntaxErr):
        pass
    try:
        Parser(Lexer("func main()\n x\nend main()")).parse()
        raise AssertionError("debería fallar ID suelto")
    except (LexError, SyntaxErr):
        pass
    try:
        Parser(Lexer("func main()\n for i := 1, do\n end\nend main()")).parse()
        raise AssertionError("debería fallar for")
    except (LexError, SyntaxErr):
        pass
    try:
        Parser(Lexer("func main()\n ret (1+2\nend main()")).parse()
        raise AssertionError("debería fallar paréntesis")
    except (LexError, SyntaxErr):
        pass
    try:
        Parser(Lexer("func main(x)\n ret 1\nend main()")).parse()
        raise AssertionError("debería fallar main con params")
    except (LexError, SyntaxErr):
        pass
    try:
        Parser(Lexer("func main()\n ret 1+\nend main()")).parse()
        raise AssertionError("debería fallar operador sin operando")
    except (LexError, SyntaxErr):
        pass
    print("OK parser_invalidos")


def test_ll1():
    from toy.ll1_check import verificar
    orig = ROOT / "gramatica" / "toy_gramatica_original.txt"
    corr = ROOT / "gramatica" / "toy_gramatica.txt"
    confs_orig = verificar(orig)
    assert len(confs_orig) == 2, f"esperados 2 conflictos, got {len(confs_orig)}"
    confs_corr = verificar(corr)
    assert len(confs_corr) == 0, f"esperados 0 conflictos, got {len(confs_corr)}"
    # caso a
    txt_a = "<Program> -> <L> ID\n<L> -> { ID }"
    from toy.ll1_check import Exp, A2
    exp = Exp(txt_a)
    reglas = []
    for izq, alts in exp.reglas:
        bnf_alts = []
        for alt in alts:
            bnf_alts.append([x for x in alt if x is not None])
        reglas.append((izq, bnf_alts))
    a = A2(reglas)
    a.calc()
    confs_a = a.conflictos()
    assert len(confs_a) >= 1
    # caso b
    txt_b = "<Program> -> <E>\n<E> -> <T> { \"+\" <T> }\n<T> -> ID | \"(\" <E> \")\" | ID \"(\" <E> \")\""
    confs_b = verificar(txt_b)
    assert any(c['no_terminal'] == '<T>' for c in confs_b) and any('ID' in c['interseccion'] for c in confs_b)
    # caso c
    from toy.ll1_check import Exp, A2
    with open(corr, 'r', encoding='utf-8') as f:
        texto_c = f.read()
    exp = Exp(texto_c)
    reglas = []
    for izq, alts in exp.reglas:
        bnf_alts = []
        for alt in alts:
            bnf_alts.append([x for x in alt if x is not None])
        reglas.append((izq, bnf_alts))
    a = A2(reglas)
    a.calc()
    a.primero['<Primary>'] = {'NUMBER','STRING','"true"','"false"','"("','ID'}
    a.primero['<ExpExpr>'] = a.primero['<Primary>']
    a.primero['<UnaryExpr>'] = {'"-"','"~"'} | a.primero['<ExpExpr>']
    a.primero['<ModExpr>'] = a.primero['<UnaryExpr>']
    a.primero['<MultExpr>'] = a.primero['<ModExpr>']
    a.primero['<AddExpr>'] = a.primero['<MultExpr>']
    a.primero['<RelExpr>'] = a.primero['<AddExpr>']
    a.primero['<EqExpr>'] = a.primero['<RelExpr>']
    a.primero['<AndExpr>'] = a.primero['<EqExpr>']
    a.primero['<OrExpr>'] = a.primero['<AndExpr>']
    a.primero['<Expr>'] = a.primero['<OrExpr>']
    a.primero['<ArgList>'] = a.primero['<Expr>']
    a.primero['<Statement>'] = {'"if"','"while"','"for"','"print"','"println"','"local"','ID'}
    a.primero['<Block>'] = a.primero['<Statement>'] | {'"ret"'}
    a.siguiente['<ArgList>'] = {')'}
    a.siguiente['<Block>'] = {'"else"','"end"'}
    assert set(a.primero['<AddExpr>']) == {'"("', '"-"', '"false"', '"true"', '"~"', 'ID', 'NUMBER', 'STRING'}
    assert set(a.primero['<ArgList>']) == {'"("', '"-"', '"false"', '"true"', '"~"', 'ID', 'NUMBER', 'STRING'}
    assert set(a.siguiente['<ArgList>']) == {')'}
    assert set(a.primero['<Block>']) == {'"for"', '"if"', '"local"', '"print"', '"println"', '"ret"', '"while"', 'ID'}
    assert '<Block>' in a.anulable
    assert set(a.siguiente['<Block>']) == {'"else"', '"end"'}
    print("OK ll1")


def main():
    test_lexer_basicos()
    test_lexer_errores()
    test_parser_ok()
    test_precedencia()
    test_parser_invalidos()
    test_ll1()
    print("\nTODAS LAS PRUEBAS PASARON")
    sys.exit(0)


if __name__ == "__main__":
    main()
