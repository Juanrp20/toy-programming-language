# run_tests.py
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))


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

    programas_rechazados = [
        "func main()\nend",
        "func main()\nend foo",
        "func f()\nend f\nfunc main()\nend main()",
        "func main()\nend main()\nfunc g()\nend",
    ]
    for prog in programas_rechazados:
        try:
            Parser(Lexer(prog)).parse()
            raise AssertionError(f"debería rechazarse programa inválido:\n{prog}")
        except (LexError, SyntaxErr):
            pass

    prog_aceptado = "func f()\nend\nfunc main()\nend main()"
    ast_aceptado = Parser(Lexer(prog_aceptado)).parse()
    assert ast_aceptado is not None and ast_aceptado[0] == "program"

    print("OK parser_invalidos")


def test_ll1():
    from toy.ll1_check import verificar, calcular_conjuntos
    orig = ROOT / "gramatica" / "toy_gramatica_original.txt"
    corr = ROOT / "gramatica" / "toy_gramatica.txt"

    with open(corr, 'r', encoding='utf-8') as f:
        texto_corr = f.read()
    anulables, primero, siguiente = calcular_conjuntos(texto_corr)

    assert set(primero['<AddExpr>']) == {'"("', '"-"', '"false"', '"true"', '"~"', 'ID', 'NUMBER', 'STRING'}
    assert set(primero['<ArgList>']) == {'"("', '"-"', '"false"', '"true"', '"~"', 'ID', 'NUMBER', 'STRING'}
    assert set(siguiente['<ArgList>']) == {'")"'}
    assert set(primero['<Block>']) == {'"for"', '"if"', '"local"', '"print"', '"println"', '"ret"', '"while"', 'ID'}
    assert '<Block>' in anulables
    assert set(siguiente['<Block>']) == {'"else"', '"end"'}

    confs_orig = verificar(orig)
    assert len(confs_orig) == 2, f"esperados 2 conflictos, got {len(confs_orig)}"
    confs_corr = verificar(corr)
    assert len(confs_corr) == 0, f"esperados 0 conflictos, got {len(confs_corr)}"

    c_l = verificar("<Program> -> <L> ID\n<L> -> { ID }")
    assert len(c_l) >= 1

    c_opt = verificar("<Program> -> [ ID ] ID")
    assert len(c_opt) >= 1

    c_paren = verificar('<Program> -> ( ID | ID "x" )')
    assert len(c_paren) >= 1

    c_a = verificar('<Program> -> <A>\n<A> -> "a" | "a" "b"')
    assert len(c_a) == 1

    corr_con_ret = texto_corr.replace("<Statement> -> <IfStmt>", "<Statement> -> <RetStmt> | <IfStmt>")
    c_ret = verificar(corr_con_ret)
    assert len(c_ret) >= 1

    with open(orig, 'r', encoding='utf-8') as f:
        texto_orig = f.read()
    orig_sin_espacios = texto_orig.replace('{ <FuncDecl> }', '{<FuncDecl>}').replace('| <RetStmt>', '|<RetStmt>')
    c_orig_sin = verificar(orig_sin_espacios)
    assert len(c_orig_sin) == 2

    print("OK ll1")


def test_afd_especificacion():
    doc_path = ROOT / "docs" / "especificacion_y_diseno_formal_del_lenguaje_toy.md"
    with open(doc_path, "r", encoding="utf-8") as f:
        md_text = f.read()

    table_lines = []
    in_table = False
    for line in md_text.splitlines():
        if "Tabla de Transiciones" in line:
            in_table = True
            continue
        if in_table:
            sline = line.strip()
            if sline.startswith("|"):
                table_lines.append(sline)
            elif table_lines:
                break

    header = [c.strip(" `") for c in table_lines[0].split("|")[1:-1]]
    table = {}
    for line in table_lines[2:]:
        cells = [c.strip() for c in line.split("|")[1:-1]]
        state = cells[0].split()[0].rstrip("*")
        table[state] = {}
        for col_name, cell in zip(header[1:], cells[1:]):
            table[state][col_name] = cell

    def char_to_col(c):
        if c == '"':
            return "DQ"
        if c == "'":
            return "SQ"
        if c == "\n":
            return "\\n"
        if c in " \t\r\v\f":
            return "Blanco"
        if c in "+-=~:><.":
            return c
        if c.isdigit():
            return "Dígito"
        if c.isalpha() or c == "_":
            return "Letra"
        if c in "*/%^(),":
            return "Símb."
        return "Otro"

    def simulate(s):
        state = "S0"
        for c in s:
            col = char_to_col(c)
            next_state = table.get(state, {}).get(col, "STOP")
            if next_state == "STOP":
                return state, False
            state = next_state
        return state, True

    casos = [
        (":=", "S12"),
        ("==", "S10"),
        ("~=", "S11"),
        ("-", "S4"),
        ("--c", "S9"),
        (">=", "S16"),
        ("<=", "S18"),
        ("+", "S19"),
        ("x", "S1"),
        ("42", "S2"),
        ("3.14", "S14"),
        ('"hola"', "S8"),
    ]

    for inp, expected in casos:
        st, ok = simulate(inp)
        assert st == expected, f"Simulación AFD falló en {inp!r}: estado={st}, esperado={expected}"

    print("OK afd_especificacion")


def test_apendice_gramatica_original():
    from toy.ll1_check import EBNFParser
    doc_path = ROOT / "docs" / "gramatica_formal_del_lenguaje_toy.md"
    txt_path = ROOT / "gramatica" / "toy_gramatica_original.txt"

    with open(doc_path, "r", encoding="utf-8") as f:
        doc = f.read()
    with open(txt_path, "r", encoding="utf-8") as f:
        txt = f.read()

    assert "# Apéndice: versión original (no LL(1))" in doc
    part = doc.split("# Apéndice: versión original (no LL(1))")[1]
    start = part.find("```ebnf")
    end = part.find("```", start + 7)
    ebnf_content = part[start + 7:end].strip()

    toks_doc = EBNFParser(ebnf_content).tokenize()
    toks_txt = EBNFParser(txt).tokenize()

    assert toks_doc == toks_txt, f"Tokens no coinciden: {len(toks_doc)} vs {len(toks_txt)}"
    print("OK apendice_gramatica_original")


def test_readme_ejemplo():
    from toy.lexer import Lexer
    from toy.parser import Parser
    readme_path = ROOT / "README.md"
    with open(readme_path, "r", encoding="utf-8") as f:
        readme = f.read()

    start = readme.find("```toy")
    assert start != -1, "No se encontró bloque ```toy en README.md"
    end = readme.find("```", start + 6)
    assert end != -1, "Bloque ```toy no está cerrado"
    code = readme[start + 6:end].strip()

    ast = Parser(Lexer(code)).parse()
    assert ast is not None and ast[0] == "program"
    print("OK readme_ejemplo")


def main():
    test_lexer_basicos()
    test_lexer_errores()
    test_parser_ok()
    test_precedencia()
    test_parser_invalidos()
    test_ll1()
    test_afd_especificacion()
    test_apendice_gramatica_original()
    test_readme_ejemplo()
    print("\nTODAS LAS PRUEBAS PASARON")
    sys.exit(0)


if __name__ == "__main__":
    main()
